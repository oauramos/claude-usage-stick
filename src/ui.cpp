#include "ui.h"
#include "config.h"
#include "hal.h"
#include <time.h>

// Shared helper — no display calls, safe before any #ifdef
static void fmtCountdown(uint32_t epoch, char* out, size_t len) {
    if (epoch == 0) {
        strlcpy(out, "--", len);
        return;
    }
    time_t now;
    time(&now);
    int32_t diff = (int32_t)epoch - (int32_t)now;
    if (diff <= 0) {
        strlcpy(out, "now", len);
        return;
    }
    int d = diff / 86400;
    int h = (diff % 86400) / 3600;
    int m = (diff % 3600) / 60;
    if (d > 0) snprintf(out, len, "%dd%dh", d, h);
    else if (h > 0) snprintf(out, len, "%dh%02dm", h, m);
    else snprintf(out, len, "%dm", m);
}

// ════════════════════════════════════════════════════════════
// OLED implementation — U8g2, SSD1306 128×64 NONAME constructor
// Physical 72×40 pixels are wired to buffer columns 30–101, rows 24–63.
// OX/OY shift local (0,0)–(71,39) coords into that buffer window.
// ════════════════════════════════════════════════════════════
#ifdef BOARD_ESP32C3_OLED

static const int OX = 30; // buffer column where visible area starts
static const int OY = 24; // buffer row where visible area starts

// Drawing helpers — all (x,y) in visible-area coords (0,0)=(top-left of 72×40)
static void oledStr(int x, int y, const char* s)        { u8g2.drawStr(OX + x, OY + y, s); }
static void oledBox(int x, int y, int w, int h)         { u8g2.drawBox(OX + x, OY + y, w, h); }
static void oledHLine(int x, int y, int w)              { u8g2.drawHLine(OX + x, OY + y, w); }
static void oledFrame(int x, int y, int w, int h)       { u8g2.drawFrame(OX + x, OY + y, w, h); }

// Draw a 3-pixel-tall progress bar (frame + 1px inner fill)
static void oledBar(int x, int y, int w, float pct) {
    oledFrame(x, y, w, 3);
    int fill = constrain((int)((w - 2) * pct / 100.0f), 0, w - 2);
    if (fill > 0) oledBox(x + 1, y + 1, fill, 1);
}

void uiInit() {
    u8g2.clearBuffer();
    u8g2.sendBuffer();
}

void uiBootProgress(int percent, const char* label) {
    u8g2.clearBuffer();

    // Title "Claude" centred — 6x10 font, 6 chars × 6px = 36px → x=18
    u8g2.setFont(u8g2_font_6x10_tr);
    oledStr(18, 10, "Claude");

    // 4px progress bar (frame + 2px fill) at y=14
    oledFrame(0, 14, 72, 4);
    int fill = constrain((int)(70 * percent / 100.0f), 0, 70);
    if (fill > 0) oledBox(1, 15, fill, 2);

    // Status label (up to 18 chars with 4x6 font)
    u8g2.setFont(u8g2_font_4x6_tr);
    char lbuf[19];
    strlcpy(lbuf, label, sizeof(lbuf));
    oledStr(0, 28, lbuf);

    // Percent right-aligned
    u8g2.setFont(u8g2_font_5x7_tr);
    char pbuf[8];
    snprintf(pbuf, sizeof(pbuf), "%d%%", percent);
    int pw = u8g2.getStrWidth(pbuf);
    oledStr(72 - pw, 37, pbuf);

    u8g2.sendBuffer();
}

void uiSetupScreen(const char* apName, const char* apPass) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_4x6_tr);

    // Header centred — "SETUP MODE" = 10 chars × 4px = 40px → x=16
    oledStr(16, 6, "SETUP MODE");
    oledHLine(0, 8, 72);

    // WiFi AP name abbreviated as "Mon-XXXX" (last 4 chars of apName)
    char wbuf[20];
    int nl = strlen(apName);
    if (nl > 4) {
        snprintf(wbuf, sizeof(wbuf), "WiFi:Mon-%s", apName + nl - 4);
    } else {
        snprintf(wbuf, sizeof(wbuf), "WiFi:%s", apName);
    }
    oledStr(0, 16, wbuf);

    // Password
    char pbuf[20];
    snprintf(pbuf, sizeof(pbuf), "Pass:%s", apPass);
    oledStr(0, 24, pbuf);

    // IP centred — "192.168.4.1" = 11 chars × 4px = 44px → x=14
    oledStr(14, 32, "192.168.4.1");

    oledStr(4, 39, "Open in browser");

    u8g2.sendBuffer();
}

void uiPinScreen(int pos, const int digits[4]) {
    u8g2.clearBuffer();

    // Header "PIN" centred — 3 chars × 5px = 15px → x=28
    u8g2.setFont(u8g2_font_5x7_tr);
    oledStr(28, 8, "PIN");

    // 4 boxes: 12px wide, 15px tall, 3px gap
    // Total: 4×12 + 3×3 = 57px  → start x = (72-57)/2 = 7
    const int BW = 12, BH = 15, BG = 3;
    const int BX0 = (72 - (4 * BW + 3 * BG)) / 2;
    const int BY  = 12;

    for (int i = 0; i < 4; i++) {
        int bx = BX0 + i * (BW + BG);
        oledFrame(bx, BY, BW, BH);
        if (i == pos) {
            // Double border for active digit
            oledFrame(bx + 1, BY + 1, BW - 2, BH - 2);
        }

        if (i < pos) {
            // Confirmed digit: show *
            oledStr(bx + 4, BY + 10, "*");
        } else if (i == pos) {
            // Active digit: show value
            char d[2] = { (char)('0' + digits[i]), '\0' };
            oledStr(bx + 4, BY + 10, d);
        }
        // Future digits: blank
    }

    // Hints
    u8g2.setFont(u8g2_font_4x6_tr);
    oledStr(0, 38, "[A]+ [B]confirm");

    u8g2.sendBuffer();
}

void uiConnecting(const char* ssid, int attempt) {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_5x7_tr);
    oledStr(0, 8, "Connecting...");

    // SSID truncated to 12 chars
    char sbuf[13];
    strlcpy(sbuf, ssid, sizeof(sbuf));
    oledStr(0, 20, sbuf);

    if (attempt > 0) {
        u8g2.setFont(u8g2_font_4x6_tr);
        char abuf[16];
        snprintf(abuf, sizeof(abuf), "Attempt %d", attempt);
        oledStr(0, 35, abuf);
    }

    u8g2.sendBuffer();
}

void uiDashboard(const UsageData& data, unsigned long lastFetchMs, int rssi, int batPct) {
    u8g2.clearBuffer();

    if (!data.ok) {
        u8g2.setFont(u8g2_font_5x7_tr);
        oledStr(0, 10, "! ERROR");
        u8g2.setFont(u8g2_font_4x6_tr);
        char ebuf[19];
        strlcpy(ebuf, data.error, sizeof(ebuf));
        oledStr(0, 24, ebuf);
        oledStr(0, 35, "[B] retry");
        u8g2.sendBuffer();
        return;
    }

    char h5rst[12], d7rst[12];
    fmtCountdown(data.h5ResetEpoch, h5rst, sizeof(h5rst));
    fmtCountdown(data.d7ResetEpoch, d7rst, sizeof(d7rst));

    u8g2.setFont(u8g2_font_5x7_tr);

    // ── Row 1: 5-hour window ─────────────────────────────
    char h5buf[10];
    snprintf(h5buf, sizeof(h5buf), "5H %.0f%%", data.h5);
    oledStr(0, 7, h5buf);
    int rw = u8g2.getStrWidth(h5rst);
    oledStr(72 - rw, 7, h5rst);
    oledBar(0, 9, 72, data.h5);

    oledHLine(0, 13, 72); // divider

    // ── Row 2: 7-day window ──────────────────────────────
    char d7buf[10];
    snprintf(d7buf, sizeof(d7buf), "7D %.0f%%", data.d7);
    oledStr(0, 20, d7buf);
    rw = u8g2.getStrWidth(d7rst);
    oledStr(72 - rw, 20, d7rst);
    oledBar(0, 22, 72, data.d7);

    oledHLine(0, 26, 72); // divider

    // ── Status row ───────────────────────────────────────
    u8g2.setFont(u8g2_font_4x6_tr);
    int ago = (int)((millis() - lastFetchMs) / 1000);
    char stbuf[22];
    if (batPct >= 0) {
        snprintf(stbuf, sizeof(stbuf), "%ddBm %ds B:%d%%", rssi, ago, batPct);
    } else {
        snprintf(stbuf, sizeof(stbuf), "%ddBm  %ds ago", rssi, ago);
    }
    oledStr(0, 33, stbuf);

    u8g2.sendBuffer();
}

void uiError(const char* title, const char* detail) {
    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_5x7_tr);
    oledStr(0, 10, title);

    if (detail) {
        u8g2.setFont(u8g2_font_4x6_tr);
        char dbuf[19];
        strlcpy(dbuf, detail, sizeof(dbuf));
        oledStr(0, 25, dbuf);
    }

    u8g2.sendBuffer();
}

void uiLockout(int attempts, int maxAttempts, int lockoutSec) {
    u8g2.setFont(u8g2_font_5x7_tr);

    for (int s = lockoutSec; s > 0; s--) {
        u8g2.clearBuffer();

        // "WRONG PIN" centred — 9 chars × 5px = 45px → x=13
        oledStr(13, 10, "WRONG PIN");

        char atbuf[12];
        snprintf(atbuf, sizeof(atbuf), "%d / %d", attempts, maxAttempts);
        int aw = u8g2.getStrWidth(atbuf);
        oledStr((72 - aw) / 2, 22, atbuf);

        char cbuf[12];
        snprintf(cbuf, sizeof(cbuf), "Wait %ds", s);
        int cw = u8g2.getStrWidth(cbuf);
        oledStr((72 - cw) / 2, 35, cbuf);

        u8g2.sendBuffer();
        delay(1000);
    }
}

// ════════════════════════════════════════════════════════════
// TFT implementation — M5StickC Plus/Plus2, LilyGo T-Display S3
// ════════════════════════════════════════════════════════════
#else

// ── Colors (RGB565) ──────────────────────────────────────
#define C_BG      TFT_BLACK
#define C_TEXT    TFT_WHITE
#define C_DIM     0x7BEF
#define C_BAR_BG  0x2104
#define C_OK      0x07E0
#define C_WARN    0xFD20
#define C_CRIT    0xF800
#define C_HEAD    0xEB87   // Claude orange
#define C_ACCENT  0xEB87
#define C_CYAN    0xF50A   // light warm orange

static uint16_t barColor(float) {
    return C_TEXT;
}

static void drawBar(int x, int y, int w, int h, float pct, const char* label) {
    lcd.setTextColor(C_TEXT, C_BG);
    lcd.setTextSize(1);
    lcd.setCursor(x, y);
    lcd.print(label);

    char ps[8];
    snprintf(ps, sizeof(ps), "%.0f%%", pct);
    lcd.setCursor(x + w - strlen(ps) * 6, y);
    lcd.setTextColor(barColor(pct), C_BG);
    lcd.print(ps);

    int by = y + 12;
    lcd.fillRect(x, by, w, h, C_BAR_BG);
    int fw = constrain((int)(w * pct / 100.0f), 0, w);
    if (fw > 0) lcd.fillRect(x, by, fw, h, barColor(pct));
}

void uiInit() {
    lcd.setRotation(SCREEN_ROT);
    lcd.fillScreen(C_BG);
}

void uiBootProgress(int percent, const char* label) {
    lcd.fillScreen(C_BG);

    lcd.setTextColor(C_ACCENT, C_BG);
    lcd.setTextSize(2);
    lcd.setCursor(30, 20);
    lcd.print("Claude Usage");

    int bx = 20, by = 60, bw = SCREEN_W - 40, bh = 14;
    lcd.fillRect(bx, by, bw, bh, C_BAR_BG);
    int fill = constrain((int)(bw * percent / 100.0f), 0, bw);
    lcd.fillRect(bx, by, fill, bh, C_ACCENT);

    char ps[8];
    snprintf(ps, sizeof(ps), "%d%%", percent);
    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(1);
    lcd.setCursor(bx + bw / 2 - strlen(ps) * 3, by + bh + 6);
    lcd.print(ps);

    lcd.setCursor(20, 100);
    lcd.print(label);
}

void uiSetupScreen(const char* apName, const char* apPass) {
    lcd.fillScreen(C_BG);

    lcd.fillRect(0, 0, SCREEN_W, 18, C_ACCENT);
    lcd.setTextColor(C_TEXT, C_ACCENT);
    lcd.setTextSize(1);
    lcd.setCursor(6, 5);
    lcd.print("SETUP MODE");

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(1);
    lcd.setCursor(10, 24);
    lcd.print("1. Connect to WiFi:");

    lcd.setTextColor(C_CYAN, C_BG);
    lcd.setTextSize(2);
    lcd.setCursor(10, 36);
    lcd.print(apName);

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(1);
    lcd.setCursor(10, 56);
    lcd.print("Password:");
    lcd.setTextColor(C_CYAN, C_BG);
    lcd.setTextSize(2);
    lcd.setCursor(10, 68);
    lcd.print(apPass);

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(1);
    lcd.setCursor(10, 92);
    lcd.print("2. Open in browser:");

    lcd.setTextColor(C_CYAN, C_BG);
    lcd.setTextSize(2);
    lcd.setCursor(10, 104);
    lcd.print("192.168.4.1");
}

void uiPinScreen(int pos, const int digits[4]) {
    lcd.fillScreen(C_BG);

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(1);
    lcd.setCursor(70, 15);
    lcd.print("UNLOCK PIN");

    int boxW = 30, boxH = 36, gap = 12;
    int startX = (SCREEN_W - (4 * boxW + 3 * gap)) / 2;
    int boxY = 40;

    for (int i = 0; i < 4; i++) {
        int x = startX + i * (boxW + gap);
        uint16_t borderCol = (i == pos) ? C_CYAN : C_DIM;

        lcd.drawRect(x, boxY, boxW, boxH, borderCol);
        if (i == pos) lcd.drawRect(x + 1, boxY + 1, boxW - 2, boxH - 2, borderCol);

        lcd.setTextSize(3);
        if (i < pos) {
            lcd.setTextColor(C_ACCENT, C_BG);
            lcd.setCursor(x + 9, boxY + 7);
            lcd.print("*");
        } else if (i == pos) {
            lcd.setTextColor(C_TEXT, C_BG);
            lcd.setCursor(x + 9, boxY + 7);
            lcd.print(digits[i]);
        }
    }

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(1);
    lcd.setCursor(20, 95);
    lcd.print("[A] cycle digit");
    lcd.setCursor(148, 95);
    lcd.print("[B] confirm");

    lcd.setCursor(35, 118);
    lcd.setTextColor(0x4A49, C_BG);
    lcd.print("Hold A+B on boot = factory reset");
}

void uiConnecting(const char* ssid, int attempt) {
    lcd.fillScreen(C_BG);
    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(1);
    lcd.setCursor(10, 40);
    lcd.print("Connecting to WiFi...");

    lcd.setTextColor(C_TEXT, C_BG);
    lcd.setTextSize(2);
    lcd.setCursor(10, 58);
    lcd.print(ssid);

    if (attempt > 0) {
        lcd.setTextColor(C_DIM, C_BG);
        lcd.setTextSize(1);
        lcd.setCursor(10, 90);
        lcd.printf("Attempt %d", attempt);
    }
}

void uiDashboard(const UsageData& data, unsigned long lastFetchMs, int rssi, int batPct) {
    lcd.fillScreen(C_BG);

    // Header
    lcd.fillRect(0, 0, SCREEN_W, 18, C_HEAD);
    lcd.setTextColor(C_TEXT, C_HEAD);
    lcd.setTextSize(1);
    lcd.setCursor(4, 5);
    lcd.print("CLAUDE USAGE");

    unsigned long ago = (millis() - lastFetchMs) / 1000;
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "%ddBm %lus", rssi, ago);
    lcd.setCursor(SCREEN_W - strlen(hdr) * 6 - 4, 5);
    lcd.print(hdr);

    if (!data.ok) {
        lcd.setTextColor(C_CRIT, C_BG);
        lcd.setTextSize(2);
        lcd.setCursor(10, 35);
        lcd.print("ERROR");
        lcd.setTextSize(1);
        lcd.setTextColor(C_DIM, C_BG);
        lcd.setCursor(10, 60);
        lcd.print(data.error);
        lcd.setCursor(10, 80);
        lcd.print("[B] retry now");
        return;
    }

    int barW = SCREEN_W - 20;
    drawBar(10, 24, barW, 10, data.h5, "5-HOUR WINDOW");
    drawBar(10, 52, barW, 10, data.d7, "7-DAY WINDOW");

    char h5rst[16], d7rst[16];
    fmtCountdown(data.h5ResetEpoch, h5rst, sizeof(h5rst));
    fmtCountdown(data.d7ResetEpoch, d7rst, sizeof(d7rst));

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(1);
    lcd.setCursor(10, 80);
    lcd.print("5H RST");
    lcd.setTextColor(C_TEXT, C_BG);
    lcd.setTextSize(2);
    lcd.setCursor(10, 92);
    lcd.print(h5rst);

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(1);
    lcd.setCursor(SCREEN_W / 2 + 10, 80);
    lcd.print("7D RST");
    lcd.setTextColor(C_TEXT, C_BG);
    lcd.setTextSize(2);
    lcd.setCursor(SCREEN_W / 2 + 10, 92);
    lcd.print(d7rst);

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(1);
    lcd.setCursor(SCREEN_W - 48, 120);
    lcd.printf("BAT %d%%", batPct);
}

void uiError(const char* title, const char* detail) {
    lcd.fillScreen(C_BG);
    lcd.setTextColor(C_CRIT, C_BG);
    lcd.setTextSize(2);
    lcd.setCursor(10, 30);
    lcd.print(title);
    if (detail) {
        lcd.setTextColor(C_DIM, C_BG);
        lcd.setTextSize(1);
        lcd.setCursor(10, 60);
        lcd.print(detail);
    }
}

void uiLockout(int attempts, int maxAttempts, int lockoutSec) {
    lcd.fillScreen(C_BG);
    lcd.setTextColor(C_CRIT, C_BG);
    lcd.setTextSize(2);
    lcd.setCursor(10, 25);
    lcd.print("WRONG PIN");

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(1);
    lcd.setCursor(10, 55);
    lcd.printf("Attempt %d of %d", attempts, maxAttempts);
    lcd.setCursor(10, 75);
    lcd.printf("Locked for %d seconds", lockoutSec);

    for (int s = lockoutSec; s > 0; s--) {
        lcd.fillRect(10, 95, 200, 20, C_BG);
        lcd.setTextColor(C_WARN, C_BG);
        lcd.setTextSize(2);
        lcd.setCursor(10, 95);
        lcd.printf("%ds", s);
        delay(1000);
    }
}

#endif // BOARD_ESP32C3_OLED
