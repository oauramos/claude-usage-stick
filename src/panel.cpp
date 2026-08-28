#include "panel.h"

#ifdef DUST_UI

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <time.h>
#include "config.h"
#include "crypto.h"
#include "settings.h"
#include "app_state.h"
#include "api.h"
#include "hal.h"
#include "ui.h"
#include "history.h"
#include "news.h"
#include "panel_html.h"

// ── Handler rules (single-threaded reentrancy contract) ──────────────────
// Handlers execute inside handleClient(), in whichever stack frame called
// panelService(): loop(), the PIN-entry poll loop, or the lockout countdown.
// 1. Never draw to the LCD, never call panelService() (recursion).
// 2. Fast NVS writes, halSetBrightness and crypto (~tens of ms) are fine.
// 3. Slow (refresh) or control-flow-destructive work (reboot, factory reset,
//    flip+redraw) is deferred via s_actions and drained by loop().
//    Sanctioned exception: /api/token probes inline — the user asked for a
//    verdict, and the screen freezing for a second is acceptable there.
// 4. A web unlock is a flag+token handoff; the screen transition happens when
//    the boot phase's stack unwinds, never from inside a handler.

static WebServer s_server(80);
static char      s_hostname[33] = "claude-usage-stick";

// Sessions: random 128-bit ids in RAM only — a reboot logs everyone out.
static const uint32_t SESSION_TTL_MS = 24UL * 3600UL * 1000UL;
struct Session {
    uint8_t  id[16];
    uint32_t lastSeenMs;
    bool     used;
};
static Session s_sessions[4];

// Web login throttle. RAM-only by design: persisting it would let an attacker
// DoS the owner, and every guess already pays the ~10k-round KDF. Web failures
// NEVER wipe the device — the 10-attempt wipe stays exclusive to the buttons.
static uint8_t  s_fails       = 0;
static uint32_t s_lockUntilMs = 0;

static bool     s_unlockPending = false;
static uint8_t  s_actions       = 0;
static uint32_t s_notBeforeMs   = 0;   // gate for REBOOT/FACTORY_RESET

// ── small helpers ────────────────────────────────────────────────────────
static void sendJson(int code, const JsonDocument& d) {
    String out;
    serializeJson(d, out);
    s_server.send(code, "application/json", out);
}

static void sendErr(int code, const char* err) {
    JsonDocument d;
    d["error"] = err;
    sendJson(code, d);
}

static bool hexToBytes(const char* hex, uint8_t* out, size_t n) {
    for (size_t i = 0; i < n * 2; i++) {
        char c = hex[i];
        uint8_t v;
        if (c >= '0' && c <= '9') v = c - '0';
        else if (c >= 'a' && c <= 'f') v = c - 'a' + 10;
        else return false;
        if (i & 1) out[i / 2] |= v; else out[i / 2] = v << 4;
    }
    return true;
}

static Session* findSession() {
    String cookie = s_server.header("Cookie");
    int at = cookie.indexOf("sid=");
    if (at < 0 || cookie.length() < at + 4 + 32) return nullptr;
    uint8_t want[16];
    if (!hexToBytes(cookie.c_str() + at + 4, want, sizeof(want))) return nullptr;
    for (auto& s : s_sessions) {
        if (!s.used || memcmp(s.id, want, sizeof(want)) != 0) continue;
        if (millis() - s.lastSeenMs > SESSION_TTL_MS) { s.used = false; return nullptr; }
        s.lastSeenMs = millis();
        return &s;
    }
    return nullptr;
}

static Session* requireAuth() {
    Session* s = findSession();
    if (!s) sendErr(401, "auth");
    return s;
}

// CSRF: cookies are SameSite=Strict AND state-changing posts must be JSON —
// a cross-origin HTML form can produce neither.
static bool requireJsonBody(JsonDocument& doc) {
    if (s_server.header("Content-Type").indexOf("application/json") < 0) {
        sendErr(400, "json_required");
        return false;
    }
    if (deserializeJson(doc, s_server.arg("plain"))) {
        sendErr(400, "bad_json");
        return false;
    }
    return true;
}

static bool pinValid(const char* pin) {
    if (strlen(pin) != 4) return false;
    for (int i = 0; i < 4; i++)
        if (!isDigit(pin[i])) return false;
    return true;
}

static void throttleFail() {
    if (++s_fails >= 5) {
        uint32_t backoffS = 60UL << (s_fails - 5);
        if (backoffS > 300) backoffS = 300;
        s_lockUntilMs = millis() + backoffS * 1000UL;
    }
}

static bool throttled(uint32_t& retryS) {
    if (s_fails < 5 || (int32_t)(s_lockUntilMs - millis()) <= 0) return false;
    retryS = (s_lockUntilMs - millis()) / 1000 + 1;
    return true;
}

// ── routes ───────────────────────────────────────────────────────────────
static void handleRoot() {
    if (s_server.header("If-None-Match") == PANEL_HTML_ETAG) {
        s_server.send(304, "text/html", "");
        return;
    }
    s_server.sendHeader("Content-Encoding", "gzip");
    s_server.sendHeader("ETag", PANEL_HTML_ETAG);
    s_server.sendHeader("Cache-Control", "no-cache");
    s_server.send_P(200, "text/html", (PGM_P)PANEL_HTML_GZ, PANEL_HTML_GZ_LEN);
}

static void handleLogin() {
    JsonDocument body;
    if (!requireJsonBody(body)) return;

    uint32_t retryS;
    if (throttled(retryS)) {
        s_server.sendHeader("Retry-After", String(retryS));
        JsonDocument d;
        d["error"]   = "throttled";
        d["retry_s"] = retryS;
        sendJson(429, d);
        return;
    }

    char pin[8];
    strlcpy(pin, body["pin"] | "", sizeof(pin));
    char tmp[256];
    // Verification IS a decrypt attempt — the PIN is never stored anywhere,
    // so the GCM tag is the only oracle (crypto.cpp).
    bool ok = pinValid(pin) && g_settings.hasBlob &&
              decryptToken(g_settings.blob, pin, tmp, sizeof(tmp));
    memset(pin, 0, sizeof(pin));

    if (!ok) {
        memset(tmp, 0, sizeof(tmp));
        throttleFail();
        sendErr(401, "auth");
        return;
    }
    s_fails = 0;

    bool wasLocked = !g_unlocked;
    strlcpy(g_token, tmp, sizeof(g_token));
    memset(tmp, 0, sizeof(tmp));
    if (wasLocked) s_unlockPending = true;   // boot phase consumes on unwind

    // Mint a session, evicting the stalest slot when all four are taken.
    Session* sess = nullptr;
    for (auto& s : s_sessions)
        if (!s.used) { sess = &s; break; }
    if (!sess) {
        sess = &s_sessions[0];
        for (auto& s : s_sessions)
            if (s.lastSeenMs < sess->lastSeenMs) sess = &s;
    }
    esp_fill_random(sess->id, sizeof(sess->id));
    sess->lastSeenMs = millis();
    sess->used = true;

    char sid[33];
    for (int i = 0; i < 16; i++) sprintf(sid + i * 2, "%02x", sess->id[i]);
    s_server.sendHeader("Set-Cookie",
        String("sid=") + sid + "; Path=/; HttpOnly; SameSite=Strict; Max-Age=86400");

    JsonDocument d;
    d["ok"] = true;
    d["unlocked_device"] = wasLocked;
    sendJson(200, d);
}

static void handleLogout() {
    Session* s = requireAuth();
    if (!s) return;
    s->used = false;
    s_server.sendHeader("Set-Cookie", "sid=; Path=/; HttpOnly; Max-Age=0");
    JsonDocument d;
    d["ok"] = true;
    sendJson(200, d);
}

static void handleState() {
    if (!requireAuth()) return;
    JsonDocument d;
    d["fw"]        = FW_VERSION;
    d["codename"]  = "Dust";
    d["hostname"]  = s_hostname;
    d["ip"]        = WiFi.localIP().toString();
    d["rssi"]      = WiFi.RSSI();
    // Internal SRAM only for both — that's the scarce pool worth watching;
    // mixing the PSRAM-inclusive counters made min > free.
    d["heap_free"] = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    d["heap_min"]  = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
    d["uptime_s"]  = millis() / 1000;
    d["locked"]    = !g_unlocked;
    d["now"]       = (uint32_t)time(nullptr);

    JsonObject u = d["usage"].to<JsonObject>();
    u["ok"]       = g_usage.ok;
    u["error"]    = g_usage.error;
    u["h5"]       = g_usage.h5;
    u["d7"]       = g_usage.d7;
    u["h5_reset"] = g_usage.h5ResetEpoch;
    u["d7_reset"] = g_usage.d7ResetEpoch;
    u["age_s"]    = (millis() - g_lastFetchMs) / 1000;

    JsonObject m = d["models"].to<JsonObject>();
    m["haiku"]  = g_models.haikuUp;
    m["sonnet"] = g_models.sonnetUp;
    m["opus"]   = g_models.opusUp;
    m["fable"]  = g_models.fableUp;
    m["ok"]     = g_models.ok;

    JsonObject c = d["settings"].to<JsonObject>();
    c["brightness"] = g_settings.brightness;
    c["poll_sec"]   = g_settings.pollSec;
    c["tz_min"]     = g_settings.tzMin;
    c["dev_name"]   = g_settings.devName;
    c["flip"]       = g_settings.flip;
    c["ui_mode"]    = g_settings.uiMode;
    c["dwell_s"]    = g_settings.dwellS;
    c["scr_mask"]   = g_settings.scrMask;
    c["mdl_mask"]   = g_settings.mdlMask;

    JsonObject nw = d["news"].to<JsonObject>();
    nw["count"]         = g_news.count;
    nw["fetched_epoch"] = g_news.fetchedAtEpoch;
    nw["ok"]            = g_news.ok;

    sendJson(200, d);
}

static void handleSettings() {
    if (!requireAuth()) return;
    JsonDocument body;
    if (!requireJsonBody(body)) return;

    JsonDocument d;
    JsonObject applied = d["applied"].to<JsonObject>();

    if (body["brightness"].is<int>()) {
        g_settings.brightness = constrain(body["brightness"].as<int>(), 0, 3);
        halSetBrightness(g_settings.brightness);   // ledcWrite — safe in a handler
        settingsPutInt("brightness", g_settings.brightness);
        applied["brightness"] = g_settings.brightness;
    }
    if (body["poll_sec"].is<int>()) {
        g_settings.pollSec = constrain(body["poll_sec"].as<int>(), MIN_POLL_SEC, MAX_POLL_SEC);
        settingsPutInt("poll_sec", g_settings.pollSec);
        applied["poll_sec"] = g_settings.pollSec;
    }
    if (body["tz_min"].is<int>()) {
        g_settings.tzMin = constrain(body["tz_min"].as<int>(), -840, 840);
        settingsApplyTZ(g_settings.tzMin);
        settingsPutInt("tz_min", g_settings.tzMin);
        applied["tz_min"] = g_settings.tzMin;
    }
    if (body["dev_name"].is<const char*>()) {
        strlcpy(g_settings.devName, body["dev_name"] | "", sizeof(g_settings.devName));
        settingsPutStr("dev_name", g_settings.devName);
        uiSetHeaderLabel(g_settings.devName);      // stores a string; next tick draws it
        applied["dev_name"] = g_settings.devName;
        d["hostname_pending_reboot"] = true;       // mDNS rename applies next boot
    }
    if (!body["flip"].isNull()) {
        g_settings.flip = (body["flip"].as<String>() == "toggle")
                              ? !g_settings.flip
                              : (body["flip"].as<int>() ? 1 : 0);
        settingsPutU8("flip", g_settings.flip);
        s_actions |= PANEL_ACT_FLIP;               // rotation + redraw happen in loop()
        applied["flip"] = g_settings.flip;
    }
    if (body["ui_mode"].is<int>()) {
        g_settings.uiMode = constrain(body["ui_mode"].as<int>(), 0, 2);
        settingsPutU8("ui_mode", g_settings.uiMode);
        applied["ui_mode"] = g_settings.uiMode;
        s_actions |= PANEL_ACT_REDRAW;             // re-pins the mode's screen
    }
    if (body["dwell_s"].is<int>()) {
        int dw = body["dwell_s"].as<int>();
        if (dw != 5 && dw != 10 && dw != 15 && dw != 30) dw = 10;
        g_settings.dwellS = (uint8_t)dw;
        settingsPutU8("dwell_s", g_settings.dwellS);
        applied["dwell_s"] = g_settings.dwellS;
    }
    if (body["scr_mask"].is<int>()) {
        uint8_t m = body["scr_mask"].as<int>() & 0x0F;
        g_settings.scrMask = m ? m : 0x01;         // carousel can never be empty
        settingsPutU8("scr_mask", g_settings.scrMask);
        applied["scr_mask"] = g_settings.scrMask;
    }
    if (body["mdl_mask"].is<int>()) {
        uint8_t m = body["mdl_mask"].as<int>() & 0x0F;
        g_settings.mdlMask = m ? m : 0x0F;
        settingsPutU8("mdl_mask", g_settings.mdlMask);
        applied["mdl_mask"] = g_settings.mdlMask;
        s_actions |= PANEL_ACT_REDRAW;
    }

    sendJson(200, d);
}

static void handleHistory() {
    if (!requireAuth()) return;
    static HistSlot slots[HIST_SLOTS];
    uint32_t newest;
    historySnapshot(slots, newest);

    // Hand-built JSON: 2×336 numbers would bloat an ArduinoJson doc for no gain.
    String out;
    out.reserve(HIST_SLOTS * 9 + 96);
    out += "{\"slot_sec\":1800,\"newest_epoch\":";
    out += newest;
    out += ",\"h5\":[";
    for (int i = 0; i < HIST_SLOTS; i++) {
        if (i) out += ',';
        if (slots[i].h5 == HIST_EMPTY) out += "null";
        else out += (int)slots[i].h5;
    }
    out += "],\"d7\":[";
    for (int i = 0; i < HIST_SLOTS; i++) {
        if (i) out += ',';
        if (slots[i].d7 == HIST_EMPTY) out += "null";
        else out += (int)slots[i].d7;
    }
    out += "]}";
    s_server.send(200, "application/json", out);
}

static void handleNews() {
    if (!requireAuth()) return;
    JsonDocument d;
    d["ok"]            = g_news.ok;
    d["fetched_epoch"] = g_news.fetchedAtEpoch;
    JsonArray arr = d["items"].to<JsonArray>();
    for (int i = 0; i < g_news.count; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["title"] = g_news.items[i].title;
        o["date"]  = g_news.items[i].date;
    }
    sendJson(200, d);
}

#ifdef PANEL_DEBUG
static void handleSeedHistory() {
    if (!requireAuth()) return;
    JsonDocument body;
    if (!requireJsonBody(body)) return;
    bool clear = body["clear"] | false;
    historySeedDemo(clear);
    s_actions |= PANEL_ACT_REDRAW;
    JsonDocument d;
    d["seeded"] = !clear;
    sendJson(200, d);
}
#endif

static void handleRefresh() {
    if (!requireAuth()) return;
    JsonDocument body;
    if (!requireJsonBody(body)) return;
    s_actions |= PANEL_ACT_REFRESH;
    JsonDocument d;
    d["queued"] = true;
    sendJson(202, d);
}

static void handleToken() {
    if (!requireAuth()) return;
    JsonDocument body;
    if (!requireJsonBody(body)) return;

    uint32_t retryS;
    if (throttled(retryS)) {
        s_server.sendHeader("Retry-After", String(retryS));
        sendErr(429, "throttled");
        return;
    }

    const char* token = body["token"] | "";
    char pin[8];
    strlcpy(pin, body["pin"] | "", sizeof(pin));
    if (strlen(token) < 10 || strlen(token) > 255 || !pinValid(pin)) {
        memset(pin, 0, sizeof(pin));
        sendErr(400, "bad_request");
        return;
    }

    // Re-authenticate the sensitive op: the PIN must still decrypt the current
    // blob. A session cookie alone can't rotate the token.
    char tmp[256];
    bool pinOk = g_settings.hasBlob && decryptToken(g_settings.blob, pin, tmp, sizeof(tmp));
    memset(tmp, 0, sizeof(tmp));
    if (!pinOk) {
        throttleFail();
        memset(pin, 0, sizeof(pin));
        sendErr(403, "wrong_pin");
        return;
    }
    s_fails = 0;

    EncryptedBlob nb;
    bool enc = encryptToken(token, pin, nb);
    memset(pin, 0, sizeof(pin));
    if (!enc) {
        sendErr(500, "encrypt_failed");
        return;
    }

    settingsPutBlob("blob", &nb, sizeof(nb));
    g_settings.blob    = nb;
    g_settings.hasBlob = true;
    strlcpy(g_token, token, sizeof(g_token));

    // Sanctioned inline probe (1–3s): the user asked for a verdict.
    fetchUsage(g_token, g_usage);
    g_lastFetchMs = millis();
    s_actions |= PANEL_ACT_REDRAW;

    JsonDocument d;
    d["saved"]    = true;
    d["probe_ok"] = g_usage.ok;
    if (!g_usage.ok) d["error"] = g_usage.error;
    sendJson(200, d);
}

static void handleWifiScan() {
    if (!requireAuth()) return;
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) {
        JsonDocument d;
        d["scanning"] = true;
        sendJson(202, d);
        return;
    }
    if (n < 0) {   // not started (or failed) — kick an async scan and report back
        WiFi.scanNetworks(true);
        JsonDocument d;
        d["scanning"] = true;
        sendJson(202, d);
        return;
    }
    JsonDocument d;
    JsonArray arr = d["networks"].to<JsonArray>();
    for (int i = 0; i < n && i < 20; i++) {
        JsonObject o = arr.add<JsonObject>();
        o["ssid"]   = WiFi.SSID(i);
        o["rssi"]   = WiFi.RSSI(i);
        o["secure"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    }
    WiFi.scanDelete();
    sendJson(200, d);
}

static void handleWifiSet() {
    if (!requireAuth()) return;
    JsonDocument body;
    if (!requireJsonBody(body)) return;
    const char* ssid = body["ssid"] | "";
    const char* pass = body["pass"] | "";
    if (!ssid[0] || strlen(ssid) > 32 || strlen(pass) > 64) {
        sendErr(400, "bad_request");
        return;
    }
    strlcpy(g_settings.ssid, ssid, sizeof(g_settings.ssid));
    strlcpy(g_settings.wifipass, pass, sizeof(g_settings.wifipass));
    settingsPutStr("ssid", g_settings.ssid);
    settingsPutStr("wifipass", g_settings.wifipass);
    s_actions |= PANEL_ACT_REBOOT;
    s_notBeforeMs = millis() + 400;   // let the response flush first
    JsonDocument d;
    d["rebooting"] = true;
    sendJson(200, d);
}

static void handleReset() {
    if (!requireAuth()) return;
    JsonDocument body;
    if (!requireJsonBody(body)) return;
    if (strcmp(body["confirm"] | "", "ERASE") != 0) {
        sendErr(400, "confirm_required");
        return;
    }
    s_actions |= PANEL_ACT_FACTORY_RESET;
    s_notBeforeMs = millis() + 400;
    JsonDocument d;
    d["erasing"] = true;
    sendJson(200, d);
}

static void handleNotFound() {
    sendErr(404, "not_found");
}

// ── public API ───────────────────────────────────────────────────────────
void panelBegin(const char* hostname) {
    strlcpy(s_hostname, hostname, sizeof(s_hostname));

    MDNS.begin(s_hostname);
    MDNS.addService("http", "tcp", 80);

    // WebServer only exposes headers listed here — Cookie carries the session.
    static const char* headerKeys[] = {"Cookie", "If-None-Match", "Content-Type"};
    s_server.collectHeaders(headerKeys, 3);

    s_server.on("/", HTTP_GET, handleRoot);
    s_server.on("/api/login", HTTP_POST, handleLogin);
    s_server.on("/api/logout", HTTP_POST, handleLogout);
    s_server.on("/api/state", HTTP_GET, handleState);
    s_server.on("/api/settings", HTTP_POST, handleSettings);
    s_server.on("/api/refresh", HTTP_POST, handleRefresh);
    s_server.on("/api/token", HTTP_POST, handleToken);
    s_server.on("/api/wifi/scan", HTTP_GET, handleWifiScan);
    s_server.on("/api/wifi", HTTP_POST, handleWifiSet);
    s_server.on("/api/reset", HTTP_POST, handleReset);
    s_server.on("/api/history", HTTP_GET, handleHistory);
    s_server.on("/api/news", HTTP_GET, handleNews);
#ifdef PANEL_DEBUG
    s_server.on("/api/debug/seed-history", HTTP_POST, handleSeedHistory);
#endif
    s_server.onNotFound(handleNotFound);
    s_server.begin();
}

void panelService() {
    s_server.handleClient();
}

bool panelUnlockPending() { return s_unlockPending; }
void panelConsumeUnlock() { s_unlockPending = false; }

uint8_t panelTakeAction() {
    uint8_t due = s_actions & (PANEL_ACT_REFRESH | PANEL_ACT_REDRAW | PANEL_ACT_FLIP);
    uint8_t gated = s_actions & (PANEL_ACT_REBOOT | PANEL_ACT_FACTORY_RESET);
    if (gated && (int32_t)(millis() - s_notBeforeMs) >= 0) due |= gated;
    s_actions &= ~due;
    return due;
}

#endif // DUST_UI
