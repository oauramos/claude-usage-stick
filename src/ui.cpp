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
