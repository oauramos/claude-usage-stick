#include "ui.h"
#include "config.h"
#include <M5StickCPlus.h>
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
    M5.Lcd.setTextColor(C_TEXT, C_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(x, y);
    M5.Lcd.print(label);

    char ps[8];
    snprintf(ps, sizeof(ps), "%.0f%%", pct);
    M5.Lcd.setCursor(x + w - strlen(ps) * 6, y);
    M5.Lcd.setTextColor(barColor(pct), C_BG);
    M5.Lcd.print(ps);

    int by = y + 12;
    M5.Lcd.fillRect(x, by, w, h, C_BAR_BG);
    int fw = constrain((int)(w * pct / 100.0f), 0, w);
    if (fw > 0) M5.Lcd.fillRect(x, by, fw, h, barColor(pct));
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
    M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(C_BG);
}

void uiBootProgress(int percent, const char* label) {
    M5.Lcd.fillScreen(C_BG);

    M5.Lcd.setTextColor(C_ACCENT, C_BG);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(30, 20);
    M5.Lcd.print("Claude Usage");

    int bx = 20, by = 60, bw = 200, bh = 14;
    M5.Lcd.fillRect(bx, by, bw, bh, C_BAR_BG);
    int fill = constrain((int)(bw * percent / 100.0f), 0, bw);
    M5.Lcd.fillRect(bx, by, fill, bh, C_ACCENT);

    char ps[8];
    snprintf(ps, sizeof(ps), "%d%%", percent);
    M5.Lcd.setTextColor(C_DIM, C_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(bx + bw / 2 - strlen(ps) * 3, by + bh + 6);
    M5.Lcd.print(ps);

    M5.Lcd.setCursor(20, 100);
    M5.Lcd.print(label);
}

void uiSetupScreen(const char* apName, const char* apPass) {
    M5.Lcd.fillScreen(C_BG);

    M5.Lcd.fillRect(0, 0, SCREEN_W, 18, C_ACCENT);
    M5.Lcd.setTextColor(C_BG, C_ACCENT);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(6, 5);
    M5.Lcd.print("SETUP MODE");

    M5.Lcd.setTextColor(C_DIM, C_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 24);
    M5.Lcd.print("1. Connect to WiFi:");

    M5.Lcd.setTextColor(C_CYAN, C_BG);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 36);
    M5.Lcd.print(apName);

    M5.Lcd.setTextColor(C_DIM, C_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 56);
    M5.Lcd.print("Password:");
    M5.Lcd.setTextColor(C_TEXT, C_BG);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(70, 56);
    M5.Lcd.print(apPass);

    M5.Lcd.setTextColor(C_DIM, C_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 80);
    M5.Lcd.print("2. Open in browser:");

    M5.Lcd.setTextColor(C_CYAN, C_BG);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 92);
    M5.Lcd.print("192.168.4.1");
}

void uiPinScreen(int pos, const int digits[4]) {
    M5.Lcd.fillScreen(C_BG);

    M5.Lcd.setTextColor(C_DIM, C_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(70, 15);
    M5.Lcd.print("UNLOCK PIN");

    int boxW = 30, boxH = 36, gap = 12;
    int startX = (SCREEN_W - (4 * boxW + 3 * gap)) / 2;
    int boxY = 40;

    for (int i = 0; i < 4; i++) {
        int x = startX + i * (boxW + gap);
        uint16_t borderCol = (i == pos) ? C_CYAN : C_DIM;

        M5.Lcd.drawRect(x, boxY, boxW, boxH, borderCol);
        if (i == pos) M5.Lcd.drawRect(x + 1, boxY + 1, boxW - 2, boxH - 2, borderCol);

        M5.Lcd.setTextSize(3);
        if (i < pos) {
            M5.Lcd.setTextColor(C_ACCENT, C_BG);
            M5.Lcd.setCursor(x + 9, boxY + 7);
            M5.Lcd.print("*");
        } else if (i == pos) {
            M5.Lcd.setTextColor(C_TEXT, C_BG);
            M5.Lcd.setCursor(x + 9, boxY + 7);
            M5.Lcd.print(digits[i]);
        }
    }

    M5.Lcd.setTextColor(C_DIM, C_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(20, 95);
    M5.Lcd.print("[A] cycle digit");
    M5.Lcd.setCursor(148, 95);
    M5.Lcd.print("[B] confirm");

    M5.Lcd.setCursor(35, 118);
    M5.Lcd.setTextColor(0x4A49, C_BG);
    M5.Lcd.print("Hold A+B on boot = factory reset");
}

void uiConnecting(const char* ssid, int attempt) {
    M5.Lcd.fillScreen(C_BG);
    M5.Lcd.setTextColor(C_DIM, C_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 40);
    M5.Lcd.print("Connecting to WiFi...");

    M5.Lcd.setTextColor(C_TEXT, C_BG);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 58);
    M5.Lcd.print(ssid);

    if (attempt > 0) {
        M5.Lcd.setTextColor(C_DIM, C_BG);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(10, 90);
        M5.Lcd.printf("Attempt %d", attempt);
    }
}

void uiDashboard(const UsageData& data, unsigned long lastFetchMs, int rssi, int batPct) {
    M5.Lcd.fillScreen(C_BG);

    // Header
    M5.Lcd.fillRect(0, 0, SCREEN_W, 18, C_HEAD);
    M5.Lcd.setTextColor(C_TEXT, C_HEAD);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(4, 5);
    M5.Lcd.print("CLAUDE USAGE");

    unsigned long ago = (millis() - lastFetchMs) / 1000;
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "%ddBm %lus", rssi, ago);
    M5.Lcd.setCursor(SCREEN_W - strlen(hdr) * 6 - 4, 5);
    M5.Lcd.print(hdr);

    if (!data.ok) {
        M5.Lcd.setTextColor(C_CRIT, C_BG);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(10, 35);
        M5.Lcd.print("ERROR");
        M5.Lcd.setTextSize(1);
        M5.Lcd.setTextColor(C_DIM, C_BG);
        M5.Lcd.setCursor(10, 60);
        M5.Lcd.print(data.error);
        M5.Lcd.setCursor(10, 80);
        M5.Lcd.print("[B] retry now");
        return;
    }

    drawBar(10, 24, 220, 10, data.h5, "5-HOUR WINDOW");
    drawBar(10, 52, 220, 10, data.d7, "7-DAY WINDOW");

    char h5rst[16], d7rst[16];
    fmtCountdown(data.h5ResetEpoch, h5rst, sizeof(h5rst));
    fmtCountdown(data.d7ResetEpoch, d7rst, sizeof(d7rst));

    M5.Lcd.setTextColor(C_DIM, C_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 80);
    M5.Lcd.print("5H RST");
    M5.Lcd.setTextColor(C_TEXT, C_BG);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 92);
    M5.Lcd.print(h5rst);

    M5.Lcd.setTextColor(C_DIM, C_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(130, 80);
    M5.Lcd.print("7D RST");
    M5.Lcd.setTextColor(C_TEXT, C_BG);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(130, 92);
    M5.Lcd.print(d7rst);

    M5.Lcd.setTextColor(C_DIM, C_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(SCREEN_W - 48, 120);
    M5.Lcd.printf("BAT %d%%", batPct);
}

void uiError(const char* title, const char* detail) {
    M5.Lcd.fillScreen(C_BG);
    M5.Lcd.setTextColor(C_CRIT, C_BG);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 30);
    M5.Lcd.print(title);
    if (detail) {
        M5.Lcd.setTextColor(C_DIM, C_BG);
        M5.Lcd.setTextSize(1);
        M5.Lcd.setCursor(10, 60);
        M5.Lcd.print(detail);
    }
}

void uiLockout(int attempts, int maxAttempts, int lockoutSec) {
    M5.Lcd.fillScreen(C_BG);
    M5.Lcd.setTextColor(C_CRIT, C_BG);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(10, 25);
    M5.Lcd.print("WRONG PIN");

    M5.Lcd.setTextColor(C_DIM, C_BG);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(10, 55);
    M5.Lcd.printf("Attempt %d of %d", attempts, maxAttempts);
    M5.Lcd.setCursor(10, 75);
    M5.Lcd.printf("Locked for %d seconds", lockoutSec);

    for (int s = lockoutSec; s > 0; s--) {
        M5.Lcd.fillRect(10, 95, 200, 20, C_BG);
        M5.Lcd.setTextColor(C_WARN, C_BG);
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(10, 95);
        M5.Lcd.printf("%ds", s);
        delay(1000);
    }
}
