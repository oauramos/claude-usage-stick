#include "news.h"

#ifdef DUST_UI

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include "config.h"
#include "certs.h"
#include "app_state.h"

NewsState g_news = {};

// ── text hygiene: entities + UTF-8 → the display's ASCII font ────────────
static size_t putMapped(char* dst, size_t len, size_t cap, uint32_t cp) {
    const char* rep = nullptr;
    char one[2] = {0, 0};
    if (cp == 0x2013 || cp == 0x2014) rep = "-";
    else if (cp == 0x2018 || cp == 0x2019) rep = "'";
    else if (cp == 0x201C || cp == 0x201D) rep = "\"";
    else if (cp == 0x2026) rep = "...";
    else if (cp == 0xA0) rep = " ";
    else if (cp >= 0x20 && cp < 0x7F) { one[0] = (char)cp; rep = one; }
    else rep = "?";
    while (*rep && len + 1 < cap) dst[len++] = *rep++;
    return len;
}

// Decodes &entities; and raw UTF-8 from a completed capture into ASCII.
static void decodeText(const char* raw, char* dst, size_t cap) {
    size_t len = 0;
    for (size_t i = 0; raw[i] && len + 1 < cap;) {
        unsigned char c = raw[i];
        if (c == '&') {
            char ent[12];
            size_t e = 0;
            size_t j = i + 1;
            while (raw[j] && raw[j] != ';' && e < sizeof(ent) - 1) ent[e++] = raw[j++];
            ent[e] = '\0';
            if (raw[j] == ';') {
                i = j + 1;
                if (!strcmp(ent, "amp")) len = putMapped(dst, len, cap, '&');
                else if (!strcmp(ent, "lt")) len = putMapped(dst, len, cap, '<');
                else if (!strcmp(ent, "gt")) len = putMapped(dst, len, cap, '>');
                else if (!strcmp(ent, "quot")) len = putMapped(dst, len, cap, '"');
                else if (!strcmp(ent, "apos")) len = putMapped(dst, len, cap, '\'');
                else if (ent[0] == '#') {
                    uint32_t cp = (ent[1] == 'x' || ent[1] == 'X')
                                      ? strtoul(ent + 2, nullptr, 16)
                                      : strtoul(ent + 1, nullptr, 10);
                    len = putMapped(dst, len, cap, cp);
                }
                continue;
            }
            // no terminator — treat as a literal ampersand
            len = putMapped(dst, len, cap, '&');
            i++;
        } else if (c < 0x80) {
            len = putMapped(dst, len, cap, (c == '\n' || c == '\t') ? ' ' : c);
            i++;
        } else {
            // raw UTF-8: 2–4 byte sequences → codepoint
            uint32_t cp = '?';
            if ((c & 0xE0) == 0xC0 && raw[i + 1]) {
                cp = ((c & 0x1F) << 6) | (raw[i + 1] & 0x3F);
                i += 2;
            } else if ((c & 0xF0) == 0xE0 && raw[i + 1] && raw[i + 2]) {
                cp = ((c & 0x0F) << 12) | ((raw[i + 1] & 0x3F) << 6) | (raw[i + 2] & 0x3F);
                i += 3;
            } else if ((c & 0xF8) == 0xF0 && raw[i + 1] && raw[i + 2] && raw[i + 3]) {
                i += 4;
            } else {
                i++;
            }
            len = putMapped(dst, len, cap, cp);
        }
    }
    dst[len] = '\0';
}

// "Wed, 27 Aug 2026 00:00:00 +0000" → "Aug 27"
static void decodeDate(const char* raw, char* dst, size_t cap) {
    char day[8] = "", mon[8] = "";
    if (sscanf(raw, "%*s %7s %7s", day, mon) == 2 && isDigit(day[0])) {
        snprintf(dst, cap, "%s %s", mon, day);
    } else {
        strlcpy(dst, "", cap);
    }
}

// ── streaming parser ─────────────────────────────────────────────────────
// Per-byte state machine, so tags and entities split across TCP reads are
// handled by construction — no sliding-window reassembly.
struct RssParser {
    enum State : uint8_t { TEXT, TAG, CDATA } state = TEXT;
    enum Capture : uint8_t { NONE, TITLE, DATE } capture = NONE;
    char     tag[24];
    uint8_t  tagLen     = 0;
    bool     tagClipped = false;
    char     raw[192];
    uint16_t rawLen    = 0;
    uint8_t  cdataTail = 0;   // matched chars of "]]>"
    bool     inItem    = false;
    NewsItem cur;
    NewsState out = {};
    bool     done = false;

    void rawPut(char c) {
        if (capture != NONE && rawLen + 1 < sizeof(raw)) raw[rawLen++] = c;
    }

    void tagComplete() {
        tag[tagLen] = '\0';
        char* sp = strchr(tag, ' ');   // strip attributes
        if (sp) *sp = '\0';
        if (!strcmp(tag, "item")) {
            inItem = true;
            memset(&cur, 0, sizeof(cur));
        } else if (!strcmp(tag, "/item")) {
            if (inItem && cur.title[0] && out.count < NEWS_MAX_ITEMS) {
                out.items[out.count++] = cur;
                if (out.count >= NEWS_MAX_ITEMS) done = true;
            }
            inItem = false;
        } else if (inItem && !strcmp(tag, "title")) {
            capture = TITLE;
            rawLen = 0;
        } else if (inItem && !strcmp(tag, "/title")) {
            raw[rawLen] = '\0';
            decodeText(raw, cur.title, sizeof(cur.title));
            capture = NONE;
        } else if (inItem && !strcmp(tag, "pubDate")) {
            capture = DATE;
            rawLen = 0;
        } else if (inItem && !strcmp(tag, "/pubDate")) {
            raw[rawLen] = '\0';
            decodeDate(raw, cur.date, sizeof(cur.date));
            capture = NONE;
        }
    }

    void feed(char c) {
        switch (state) {
            case TEXT:
                if (c == '<') {
                    state = TAG;
                    tagLen = 0;
                    tagClipped = false;
                } else {
                    rawPut(c);
                }
                break;
            case TAG:
                if (c == '>') {
                    if (!tagClipped) tagComplete();
                    state = TEXT;
                } else if (tagLen < sizeof(tag) - 1) {
                    tag[tagLen++] = c;
                    // "<![CDATA[" opens raw content that may contain '<' and '>'
                    if (tagLen == 8 && !memcmp(tag, "![CDATA[", 8)) {
                        state = CDATA;
                        cdataTail = 0;
                    }
                } else {
                    tagClipped = true;   // long/uninteresting tag — skip to '>'
                }
                break;
            case CDATA:
                if (c == ']' && cdataTail < 2) {
                    cdataTail++;
                } else if (c == '>' && cdataTail == 2) {
                    state = TEXT;
                    cdataTail = 0;
                } else {
                    while (cdataTail) { rawPut(']'); cdataTail--; }
                    rawPut(c);
                }
                break;
        }
    }
};

static bool newsFetch() {
    WiFiClientSecure client;
    client.setCACert(CA_BUNDLE);
    HTTPClient https;
    https.setTimeout(NEWS_TIMEOUT_MS);
    if (!https.begin(client, NEWS_FEED_URL)) return false;
    // Force HTTP/1.0: getStreamPtr() bypasses HTTPClient's chunked-transfer
    // decoding, and raw chunk-size lines would corrupt the byte stream.
    https.useHTTP10(true);
    if (https.GET() != HTTP_CODE_OK) {
        https.end();
        return false;
    }

    WiFiClient* stream = https.getStreamPtr();
    RssParser p;
    uint32_t deadline = millis() + NEWS_TIMEOUT_MS + 5000;
    while (!p.done && (int32_t)(deadline - millis()) > 0) {
        if (!stream->available()) {
            if (!stream->connected()) break;
            delay(2);
            continue;
        }
        uint8_t buf[256];
        int n = stream->read(buf, sizeof(buf));
        for (int i = 0; i < n && !p.done; i++) p.feed((char)buf[i]);
    }
    https.end();   // abandoning the TLS session mid-body is fine

    if (p.out.count == 0) return false;
    p.out.ok = true;
    p.out.fetchedAtEpoch = (uint32_t)time(nullptr);
    g_news = p.out;
    return true;
}

bool newsTick() {
    static uint32_t nextAtMs = 0;
    if (g_lastFetchMs == 0) return false;   // let the first usage poll land first
    if (nextAtMs == 0) nextAtMs = millis() + 15000;
    if ((int32_t)(millis() - nextAtMs) < 0) return false;

    bool ok = newsFetch();
    if (!ok) g_news.ok = false;   // keep stale items, flag the failed attempt
    nextAtMs = millis() + (ok ? NEWS_POLL_SEC : NEWS_RETRY_SEC) * 1000UL;
    return ok;
}

#endif // DUST_UI
