#include "ui.h"
#include "config.h"
#include "hal.h"
#include <time.h>

// ── Colors (RGB565) ──────────────────────────────────────
#define C_BG      TFT_BLACK
#define C_TEXT    TFT_WHITE
#define C_DIM     0x7BEF
#define C_BAR_BG  0x2104
#define C_OK      0x07E0
#define C_WARN    0xFD20
#define C_CRIT    0xF800
#define C_HEAD    0xEB87   // Claude orange
#define C_ACCENT  0xEB87   // Claude orange
#define C_CYAN    0xF50A   // light warm orange

// The base layout is designed for the 320x170 LCD. On the larger AMOLED panel
// (536x240) everything is scaled up so text stays readable. Coordinates are
// expressed in base units and mapped through these macros at draw time.
#ifdef BOARD_TDISPLAY_S3_AMOLED
  #define UI_SCALE   2
#else
  #define UI_SCALE   1
#endif
#define TS(n) ((n) * UI_SCALE)
#define SX(n) ((n) * UI_SCALE)
#define SY(n) ((n) * UI_SCALE)

static uint16_t barColor(float) {
    return C_TEXT;
}

static void drawBar(int x, int y, int w, int h, float pct, const char* label) {
    lcd.setTextColor(C_TEXT, C_BG);
    lcd.setTextSize(TS(1));
    lcd.setCursor(x, y);
    lcd.print(label);

    char ps[8];
    snprintf(ps, sizeof(ps), "%.0f%%", pct);
    lcd.setCursor(x + w - strlen(ps) * TS(6), y);
    lcd.setTextColor(barColor(pct), C_BG);
    lcd.print(ps);

    int by = y + SY(12);
    lcd.fillRect(x, by, w, h, C_BAR_BG);
    int fw = constrain((int)(w * pct / 100.0f), 0, w);
    if (fw > 0) lcd.fillRect(x, by, fw, h, barColor(pct));
}

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

void uiInit() {
    lcd.setRotation(SCREEN_ROT);
    halClear(C_BG);
    halFlush();
}

void uiBootProgress(int percent, const char* label) {
    halClear(C_BG);

    lcd.setTextColor(C_ACCENT, C_BG);
    lcd.setTextSize(TS(2));
    lcd.setCursor(SX(30), SY(20));
    lcd.print("Claude Usage");

    int bx = SX(20), by = SY(60), bw = SCREEN_W - SX(40), bh = SY(14);
    lcd.fillRect(bx, by, bw, bh, C_BAR_BG);
    int fill = constrain((int)(bw * percent / 100.0f), 0, bw);
    lcd.fillRect(bx, by, fill, bh, C_ACCENT);

    char ps[8];
    snprintf(ps, sizeof(ps), "%d%%", percent);
    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(TS(1));
    lcd.setCursor(bx + bw / 2 - strlen(ps) * TS(3), by + bh + SY(6));
    lcd.print(ps);

    lcd.setCursor(SX(20), SY(100));
    lcd.print(label);
    halFlush();
}

void uiSetupScreen(const char* apName, const char* apPass) {
    halClear(C_BG);

    lcd.fillRect(0, 0, SCREEN_W, SY(18), C_ACCENT);
    lcd.setTextColor(C_TEXT, C_ACCENT);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SX(6), SY(5));
    lcd.print("SETUP MODE");

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SX(10), SY(24));
    lcd.print("1. Connect to WiFi:");

    lcd.setTextColor(C_CYAN, C_BG);
    lcd.setTextSize(TS(2));
    lcd.setCursor(SX(10), SY(36));
    lcd.print(apName);

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SX(10), SY(56));
    lcd.print("Password:");
    lcd.setTextColor(C_CYAN, C_BG);
    lcd.setTextSize(TS(2));
    lcd.setCursor(SX(10), SY(68));
    lcd.print(apPass);

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SX(10), SY(92));
    lcd.print("2. Open in browser:");

    lcd.setTextColor(C_CYAN, C_BG);
    lcd.setTextSize(TS(2));
    lcd.setCursor(SX(10), SY(104));
    lcd.print("192.168.4.1");
    halFlush();
}

void uiPinScreen(int pos, const int digits[4]) {
    halClear(C_BG);

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SX(70), SY(15));
    lcd.print("UNLOCK PIN");

    int boxW = SX(30), boxH = SY(36), gap = SX(12);
    int startX = (SCREEN_W - (4 * boxW + 3 * gap)) / 2;
    int boxY = SY(40);

    for (int i = 0; i < 4; i++) {
        int x = startX + i * (boxW + gap);
        uint16_t borderCol = (i == pos) ? C_CYAN : C_DIM;

        lcd.drawRect(x, boxY, boxW, boxH, borderCol);
        if (i == pos) lcd.drawRect(x + 1, boxY + 1, boxW - 2, boxH - 2, borderCol);

        lcd.setTextSize(TS(3));
        if (i < pos) {
            lcd.setTextColor(C_ACCENT, C_BG);
            lcd.setCursor(x + SX(9), boxY + SY(7));
            lcd.print("*");
        } else if (i == pos) {
            lcd.setTextColor(C_TEXT, C_BG);
            lcd.setCursor(x + SX(9), boxY + SY(7));
            lcd.print(digits[i]);
        }
    }

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SX(20), SY(95));
    lcd.print("[A] cycle digit");
    lcd.setCursor(SX(148), SY(95));
    lcd.print("[B] confirm");

    lcd.setCursor(SX(35), SY(118));
    lcd.setTextColor(0x4A49, C_BG);
    lcd.print("Hold A+B on boot = factory reset");
    halFlush();
}

void uiConnecting(const char* ssid, int attempt) {
    halClear(C_BG);
    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SX(10), SY(40));
    lcd.print("Connecting to WiFi...");

    lcd.setTextColor(C_TEXT, C_BG);
    lcd.setTextSize(TS(2));
    lcd.setCursor(SX(10), SY(58));
    lcd.print(ssid);

    if (attempt > 0) {
        lcd.setTextColor(C_DIM, C_BG);
        lcd.setTextSize(TS(1));
        lcd.setCursor(SX(10), SY(90));
        lcd.printf("Attempt %d", attempt);
    }
    halFlush();
}

void uiDashboard(const UsageData& data, unsigned long lastFetchMs, int rssi, int batPct) {
    halClear(C_BG);

    // Header
    lcd.fillRect(0, 0, SCREEN_W, SY(18), C_HEAD);
    lcd.setTextColor(C_TEXT, C_HEAD);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SX(4), SY(5));
    lcd.print("CLAUDE USAGE");

    unsigned long ago = (millis() - lastFetchMs) / 1000;
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "%ddBm %lus", rssi, ago);
    lcd.setCursor(SCREEN_W - strlen(hdr) * TS(6) - SX(4), SY(5));
    lcd.print(hdr);

    if (!data.ok) {
        lcd.setTextColor(C_CRIT, C_BG);
        lcd.setTextSize(TS(2));
        lcd.setCursor(SX(10), SY(35));
        lcd.print("ERROR");
        lcd.setTextSize(TS(1));
        lcd.setTextColor(C_DIM, C_BG);
        lcd.setCursor(SX(10), SY(60));
        lcd.print(data.error);
        lcd.setCursor(SX(10), SY(80));
        lcd.print("[B] retry now");
        halFlush();
        return;
    }

    int barW = SCREEN_W - SX(20);
    drawBar(SX(10), SY(24), barW, SY(10), data.h5, "5-HOUR WINDOW");
    drawBar(SX(10), SY(52), barW, SY(10), data.d7, "7-DAY WINDOW");

    char h5rst[16], d7rst[16];
    fmtCountdown(data.h5ResetEpoch, h5rst, sizeof(h5rst));
    fmtCountdown(data.d7ResetEpoch, d7rst, sizeof(d7rst));

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SX(10), SY(80));
    lcd.print("5H RST");
    lcd.setTextColor(C_TEXT, C_BG);
    lcd.setTextSize(TS(2));
    lcd.setCursor(SX(10), SY(92));
    lcd.print(h5rst);

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SCREEN_W / 2 + SX(10), SY(80));
    lcd.print("7D RST");
    lcd.setTextColor(C_TEXT, C_BG);
    lcd.setTextSize(TS(2));
    lcd.setCursor(SCREEN_W / 2 + SX(10), SY(92));
    lcd.print(d7rst);

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SCREEN_W - SX(48), SY(120));
    lcd.printf("BAT %d%%", batPct);
    halFlush();
}

void uiError(const char* title, const char* detail) {
    halClear(C_BG);
    lcd.setTextColor(C_CRIT, C_BG);
    lcd.setTextSize(TS(2));
    lcd.setCursor(SX(10), SY(30));
    lcd.print(title);
    if (detail) {
        lcd.setTextColor(C_DIM, C_BG);
        lcd.setTextSize(TS(1));
        lcd.setCursor(SX(10), SY(60));
        lcd.print(detail);
    }
    halFlush();
}

void uiLockout(int attempts, int maxAttempts, int lockoutSec) {
    halClear(C_BG);
    lcd.setTextColor(C_CRIT, C_BG);
    lcd.setTextSize(TS(2));
    lcd.setCursor(SX(10), SY(25));
    lcd.print("WRONG PIN");

    lcd.setTextColor(C_DIM, C_BG);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SX(10), SY(55));
    lcd.printf("Attempt %d of %d", attempts, maxAttempts);
    lcd.setCursor(SX(10), SY(75));
    lcd.printf("Locked for %d seconds", lockoutSec);

    for (int s = lockoutSec; s > 0; s--) {
        lcd.fillRect(SX(10), SY(95), SX(200), SY(20), C_BG);
        lcd.setTextColor(C_WARN, C_BG);
        lcd.setTextSize(TS(2));
        lcd.setCursor(SX(10), SY(95));
        lcd.printf("%ds", s);
        halFlush();
        delay(1000);
    }
}
