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

void uiSetupScreen(const char* apName, const char* apPass, bool reconfigure) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_4x6_tr);

    // Header centred — "SETUP MODE" = 10 chars × 4px = 40px → x=16.
    // No room for a reconfigure variant on 72×40; the portal page says it.
    (void)reconfigure;
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

// The OLED redraws its whole frame each tick, so Static only latches the
// attempt counts the ticks need.
static int s_lockAttempts = 0, s_lockMax = 0;

void uiLockoutStatic(int attempts, int maxAttempts, int lockoutSec) {
    (void)lockoutSec;
    s_lockAttempts = attempts;
    s_lockMax      = maxAttempts;
}

void uiLockoutTick(int secondsLeft) {
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.clearBuffer();

    // "WRONG PIN" centred — 9 chars × 5px = 45px → x=13
    oledStr(13, 10, "WRONG PIN");

    char atbuf[12];
    snprintf(atbuf, sizeof(atbuf), "%d / %d", s_lockAttempts, s_lockMax);
    int aw = u8g2.getStrWidth(atbuf);
    oledStr((72 - aw) / 2, 22, atbuf);

    char cbuf[12];
    snprintf(cbuf, sizeof(cbuf), "Wait %ds", secondsLeft);
    int cw = u8g2.getStrWidth(cbuf);
    oledStr((72 - cw) / 2, 35, cbuf);

    u8g2.sendBuffer();
}

void uiDashboardClock(const UsageData& data, unsigned long lastFetchMs, int rssi) {
    // The OLED redraws fully without any visible flicker, so just refresh everything.
    uiDashboard(data, lastFetchMs, rssi, halBatPercent());
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
#define C_HEAD_DK 0xA244   // dimmed Claude orange — empty wifi bars, hairline dividers

// The base layout is designed for the ~240x135 LCD. Larger panels scale the
// coordinates and font up so text stays readable and the layout fills the screen.
// Coordinates are expressed in base units and mapped through these macros at draw time.
#if defined(BOARD_TDISPLAY_S3_AMOLED)
  // 536x240 panel — uniform 2x of the base layout.
  #define TS(n) ((n) * 2)
  #define SX(n) ((n) * 2)
  #define SY(n) ((n) * 2)
#elif defined(BOARD_CROWPANEL_ADV_35)
  // 480x320 panel — 2x font, with coordinates stretched to fill the whole screen
  // (~2x across, ~2.37x down) so the dashboard spreads over the full height
  // instead of bunching up at the top.
  #define TS(n) ((n) * 2)
  #define SX(n) ((int)((n) * (SCREEN_W / 240.0f)))
  #define SY(n) ((int)((n) * (SCREEN_H / 135.0f)))
#else
  #define TS(n) (n)
  #define SX(n) (n)
  #define SY(n) (n)
#endif

#ifdef BOARD_CROWPANEL_ADV_35
// The CrowPanel's ILI9488 is too slow to clear-then-redraw on screen without flicker.
// The dashboard is therefore rendered into an off-screen sprite (PSRAM) and pushed in a
// single transfer — no flicker. Other screens draw straight to the panel so touch input
// (PIN entry) stays snappy. TFT_eSprite hides (not overrides virtually) the TFT_eSPI
// drawing methods, so dashboard drawing must use a TFT_eSprite-typed target (see drawBar
// being a template that binds to the concrete type at compile time).
static TFT_eSprite s_dash = TFT_eSprite(&lcd);
static bool        s_dashReady = false;
static TFT_eSprite& dashTarget() {
    if (!s_dashReady) { s_dash.setColorDepth(16); s_dash.createSprite(SCREEN_W, SCREEN_H); s_dashReady = true; }
    return s_dash;
}
  #define UI_PUSH_DASH() (s_dash.pushSprite(0, 0))
#else
  #define UI_PUSH_DASH() halFlush()
#endif

static uint16_t barColor(float) {
    return C_TEXT;
}

// Right-aligned reset countdown that rides on a usage bar's label row (the
// M5StickC Plus 240x135 layout). A fixed-width slot is cleared first so a shorter
// string (e.g. "59m" replacing "1h59m") leaves no trailing pixels when the 10s
// clock tick repaints it in place. Only ever called on the 240px panel, so the
// slot width is tuned for it. Templated like drawBar to bind to the real target.
static const int RESET_SLOT_W = 42;
template <class GFX>
static void drawResetSlot(GFX& g, int barX, int barW, int y, const char* reset) {
    g.fillRect(barX + barW - RESET_SLOT_W, y, RESET_SLOT_W, 8, C_BG);
    g.setTextColor(C_DIM, C_BG);
    g.setTextSize(1);
    g.setCursor(barX + barW - (int)strlen(reset) * 6, y);
    g.print(reset);
}

// Templated so it binds to the concrete target type (TFT_eSPI panel or TFT_eSprite
// buffer) — those share method names but are not virtual, so a base reference would
// dispatch to the wrong (panel) implementation.
// When `reset` is given (M5StickC Plus), the countdown rides at the right of the
// label row and the % is tucked just left of it; otherwise the % is flush-right
// and every other board renders exactly as before.
template <class GFX>
static void drawBar(GFX& g, int x, int y, int w, int h, float pct, const char* label,
                    const char* reset = nullptr) {
    g.setTextColor(C_TEXT, C_BG);
    g.setTextSize(TS(1));
    g.setCursor(x, y);
    g.print(label);

    int pctRight = x + w;
    if (reset) {
        drawResetSlot(g, x, w, y, reset);
        pctRight = x + w - RESET_SLOT_W - TS(4);
    }

    char ps[8];
    snprintf(ps, sizeof(ps), "%.0f%%", pct);
    g.setCursor(pctRight - (int)strlen(ps) * TS(6), y);
    g.setTextColor(barColor(pct), C_BG);
    g.print(ps);

    int by = y + SY(12);
    g.fillRect(x, by, w, h, C_BAR_BG);
    int fw = constrain((int)(w * pct / 100.0f), 0, w);
    if (fw > 0) g.fillRect(x, by, fw, h, barColor(pct));
}

#ifdef MANGO_UI
static ModelStatus s_modelStatus = {true, true, true, true, false};

void uiSetModelStatus(const ModelStatus& s) {
    s_modelStatus = s;
}

void uiApplyRotation(bool flipped) {
    // 1 and 3 are the two landscape orientations; XOR-2 toggles between them
    // regardless of which one is this board's default (S3 = 1, M5StickC Plus = 3).
    lcd.setRotation(flipped ? (SCREEN_ROT ^ 2) : SCREEN_ROT);
    halClear(C_BG);
}

#ifdef BOARD_TDISPLAY_S3
// The header's left text alternates between the device label and the panel URL
// on the 10s clock tick. Both strings print opaque at a fixed width so each
// phase fully covers the other without a fillRect (no flicker). 28 chars stops
// short of the right cluster's worst-case extent (~x187).
static char s_netUrl[40]      = "";
static char s_hdrLabel[29]    = "CLAUDE USAGE";
static bool s_hdrShowUrl      = false;

void uiSetNetInfo(const char* url) {
    strlcpy(s_netUrl, url, sizeof(s_netUrl));
}

void uiSetHeaderLabel(const char* name) {
    if (!name || !name[0]) return;
    strlcpy(s_hdrLabel, name, sizeof(s_hdrLabel));
    for (char* p = s_hdrLabel; *p; p++) *p = toupper((unsigned char)*p);
}

template <class GFX>
static void drawHeaderLeft(GFX& g) {
    g.setTextColor(C_TEXT, C_HEAD);
    g.setTextSize(1);
    g.setCursor(4, 5);
    g.printf("%-28.28s", (s_hdrShowUrl && s_netUrl[0]) ? s_netUrl : s_hdrLabel);
}
#endif // BOARD_TDISPLAY_S3

// Clawd, 18x5 px (MSB = leftmost column). The row-1 gaps at cols 5/12 are the eyes.
// Shared by the S3 mascot row and the M5StickC Plus status-panel mascot.
static const uint32_t CLAWD_ROWS[5] = {
    0b000111111111111000,
    0b000110111111011000,
    0b011111111111111110,
    0b000111111111111000,
    0b000010100001010000,
};
static const uint32_t CLAWD_DEAD_ROW1 = 0b000111111111111000;   // eyes filled solid

// Bitmap columns of Clawd's two eyes (the row-1 gaps) — used by both blink ticks.
static const int CLAWD_EYE_COLS[2] = {5, 12};

// Left edge (in px from the mascot's x) of bitmap column c when Clawd is W px
// wide. Rounding to nearest lets W be a non-multiple of 18 (fractional cell
// widths) while keeping the edges symmetric, so both eyes render the same width.
static inline int mascotEdge(int c, int W) { return (c * W + 9) / 18; }

// W = total width; rh = row height, ~2x the cell width (W/18) — terminal quadrant
// cells are about twice as tall as wide, and square cells squash him.
static void drawMascot(TFT_eSPI& g, int x, int y, int W, int rh, uint16_t color, bool dead) {
    for (int r = 0; r < 5; r++) {
        uint32_t row = (dead && r == 1) ? CLAWD_DEAD_ROW1 : CLAWD_ROWS[r];
        for (int c = 0; c < 18; c++)
            if (row & (1UL << (17 - c)))
                g.fillRect(x + mascotEdge(c, W), y + r * rh,
                           mascotEdge(c + 1, W) - mascotEdge(c, W), rh, color);
    }
    if (dead) {
        for (int e = 0; e < 2; e++) {
            int cx = x + (mascotEdge(CLAWD_EYE_COLS[e], W) +
                          mascotEdge(CLAWD_EYE_COLS[e] + 1, W)) / 2;
            int cy = y + rh + rh / 2;
            g.drawLine(cx - 3, cy - 4, cx + 3, cy + 4, C_BG);
            g.drawLine(cx - 2, cy - 4, cx + 4, cy + 4, C_BG);
            g.drawLine(cx + 3, cy - 4, cx - 3, cy + 4, C_BG);
            g.drawLine(cx + 4, cy - 4, cx - 2, cy + 4, C_BG);
        }
    }
}

// ── Model-health panel ──────────────────────────────────────────────────────
// The space below the bars differs by screen size:
//   • T-Display S3 (320x170): the reset countdowns sit on their own size-2 row
//     below the bars (the Clarity layout — easier to read), then a full row of
//     four labelled, blinking Clawds — no divider, the mascots speak for themselves.
//   • M5StickC Plus (240x135): reset rides on the bar rows (no room below);
//     a "── MODELS ──" divider, one overall-health Clawd + a 2x2 "NAME STATUS" grid.
// drawStatusPanel() is the board-specific entry point either way, so uiDashboard
// just calls it.

#ifdef BOARD_TDISPLAY_S3
// ── T-Display S3: reset row below the bars + four labelled Clawds ───────────
// Reset countdowns get their own row under the bars; the "MODELS" divider and
// the four mascots — each named, each blinking when healthy — fill what's left.
#define RESET_CAP_Y     80
#define RESET_VAL_Y     92
#define MASCOT_W        44                // fractional ~2.4px cells via mascotEdge
#define MASCOT_RH       5                 // row height → 25px tall
#define MASCOT_Y        122
#define MASCOT_SPACING  80
#define MASCOT_CX0      40
#define MASCOT_NAME_Y   156
#define MASCOT_CX(i) (MASCOT_CX0 + (i) * MASCOT_SPACING)
#define MASCOT_X(i)  (MASCOT_CX(i) - MASCOT_W / 2)

// Size-2 countdown values — padded, opaque print overwrites in place so the
// 10s clock tick can repaint them without clearing first.
static void drawResetValues(TFT_eSPI& g, const char* h5rst, const char* d7rst) {
    g.setTextColor(C_TEXT, C_BG);
    g.setTextSize(2);
    g.setCursor(10, RESET_VAL_Y);
    g.printf("%-8s", h5rst);
    g.setCursor(SCREEN_W / 2 + 10, RESET_VAL_Y);
    g.printf("%-8s", d7rst);
}

static void drawResetRow(TFT_eSPI& g, const char* h5rst, const char* d7rst) {
    g.setTextColor(C_DIM, C_BG);
    g.setTextSize(1);
    g.setCursor(10, RESET_CAP_Y);
    g.print("5H RESET");
    g.setCursor(SCREEN_W / 2 + 10, RESET_CAP_Y);
    g.print("7D RESET");
    drawResetValues(g, h5rst, d7rst);
}

static void drawStatusPanel(TFT_eSPI& g) {
    static const char* names[4] = {"HAIKU", "SONNET", "OPUS", "FABLE"};
    bool up[4] = {s_modelStatus.haikuUp, s_modelStatus.sonnetUp,
                  s_modelStatus.opusUp,  s_modelStatus.fableUp};
    for (int i = 0; i < 4; i++) {
        int cx = MASCOT_CX(i);
        // Unknown (status never fetched) renders gray without X eyes, so a
        // status-page outage is never mistaken for a model outage.
        bool dead = s_modelStatus.ok && !up[i];
        uint16_t col = (!s_modelStatus.ok || dead) ? C_DIM : C_HEAD;
        drawMascot(g, MASCOT_X(i), MASCOT_Y, MASCOT_W, MASCOT_RH, col, dead);
        g.setTextColor(C_DIM, C_BG);
        g.setTextSize(1);
        g.setCursor(cx - (int)strlen(names[i]) * 3, MASCOT_NAME_Y);
        g.print(names[i]);
    }
}

// Repaint only the eye cells of the healthy mascots — drawn straight to the panel,
// so the 2s "I'm alive" blink costs no full redraw.
void uiBlinkTick(bool closed) {
    bool up[4] = {s_modelStatus.haikuUp, s_modelStatus.sonnetUp,
                  s_modelStatus.opusUp,  s_modelStatus.fableUp};
    int ey = MASCOT_Y + MASCOT_RH;   // eye row 1
    for (int i = 0; i < 4; i++) {
        if (!s_modelStatus.ok || !up[i]) continue;   // dead/unknown don't blink
        for (int e = 0; e < 2; e++) {
            int ex = MASCOT_X(i) + mascotEdge(CLAWD_EYE_COLS[e], MASCOT_W);
            int ew = mascotEdge(CLAWD_EYE_COLS[e] + 1, MASCOT_W) -
                     mascotEdge(CLAWD_EYE_COLS[e], MASCOT_W);
            if (closed) {
                lcd.fillRect(ex, ey, ew, MASCOT_RH, C_HEAD);                  // lid down
                lcd.fillRect(ex, ey + MASCOT_RH / 2 - 1, ew, 2, C_BG);        // shut line
            } else {
                lcd.fillRect(ex, ey, ew, MASCOT_RH, C_BG);                    // eye open
            }
        }
    }
}

#else
// ── M5StickC Plus 240x135: one overall-health Clawd + a 2x2 text grid ───────
#define PANEL_MASCOT_S  2
#define PANEL_MASCOT_X  22
#define PANEL_MASCOT_Y  100
#define PANEL_CAP_Y     80
#define PANEL_LINE_Y    84
#define PANEL_COL0_X    76
#define PANEL_COL1_X    164
#define PANEL_ROW0_Y    98
#define PANEL_ROW1_Y    114

static void drawModelsDivider(TFT_eSPI& g, int capY, int lineY) {
    const char* cap = "MODELS";
    int capW = (int)strlen(cap) * 6;
    int cx   = SCREEN_W / 2;
    g.setTextSize(1);
    g.setTextColor(C_DIM, C_BG);
    g.setCursor(cx - capW / 2, capY);
    g.print(cap);
    g.drawFastHLine(14, lineY, (cx - capW / 2 - 6) - 14, C_HEAD_DK);
    g.drawFastHLine(cx + capW / 2 + 6, lineY,
                    (SCREEN_W - 14) - (cx + capW / 2 + 6), C_HEAD_DK);
}

static void drawStatusPanel(TFT_eSPI& g) {
    static const char* names[4] = {"HAIKU", "SONNET", "OPUS", "FABLE"};
    bool up[4] = {s_modelStatus.haikuUp, s_modelStatus.sonnetUp,
                  s_modelStatus.opusUp,  s_modelStatus.fableUp};
    drawModelsDivider(g, PANEL_CAP_Y, PANEL_LINE_Y);

    // One Clawd to the left of the grid reflects overall health: Claude orange when
    // every model is up, gray with X-eyes if any is down, gray (no X) until fetched.
    bool anyDown = false;
    for (int i = 0; i < 4; i++) anyDown = anyDown || !up[i];
    bool dead = s_modelStatus.ok && anyDown;
    drawMascot(g, PANEL_MASCOT_X, PANEL_MASCOT_Y, 18 * PANEL_MASCOT_S, PANEL_MASCOT_S * 2,
               (!s_modelStatus.ok || dead) ? C_DIM : C_HEAD, dead);

    const int colX[2] = {PANEL_COL0_X, PANEL_COL1_X};
    const int rowY[2] = {PANEL_ROW0_Y, PANEL_ROW1_Y};
    g.setTextSize(1);
    for (int i = 0; i < 4; i++) {
        const char* st;
        uint16_t col;
        if (!s_modelStatus.ok) { st = "?";    col = C_DIM;  }   // status never fetched
        else if (up[i])        { st = "UP";   col = C_HEAD; }   // Claude orange = healthy
        else                   { st = "DOWN"; col = C_DIM;  }   // gray = down
        char buf[16];
        snprintf(buf, sizeof(buf), "%-6s %s", names[i], st);   // padded name aligns the status column
        g.setTextColor(col, C_BG);
        g.setCursor(colX[i % 2], rowY[i / 2]);
        g.print(buf);
    }
}

// Blink the panel mascot's eyes — only when he's the healthy (orange) Clawd; the
// gray "something's down"/unknown Clawd stays still. The 2s liveness blink.
void uiBlinkTick(bool closed) {
    bool up[4] = {s_modelStatus.haikuUp, s_modelStatus.sonnetUp,
                  s_modelStatus.opusUp,  s_modelStatus.fableUp};
    bool anyDown = false;
    for (int i = 0; i < 4; i++) anyDown = anyDown || !up[i];
    if (!s_modelStatus.ok || anyDown) return;

    int ch = PANEL_MASCOT_S * 2;
    int ey = PANEL_MASCOT_Y + ch;   // eye row 1
    for (int e = 0; e < 2; e++) {
        int ex = PANEL_MASCOT_X + CLAWD_EYE_COLS[e] * PANEL_MASCOT_S;
        if (closed) {
            lcd.fillRect(ex, ey, PANEL_MASCOT_S, ch, C_HEAD);           // lid down
            lcd.fillRect(ex, ey + ch / 2 - 1, PANEL_MASCOT_S, 2, C_BG); // shut line
        } else {
            lcd.fillRect(ex, ey, PANEL_MASCOT_S, ch, C_BG);            // eye open
        }
    }
}
#endif // BOARD_TDISPLAY_S3 four-mascot row vs M5StickC Plus status panel

static void drawWifiIcon(TFT_eSPI& g, int x, int rssi) {
    int level = (rssi >= -55) ? 4 : (rssi >= -65) ? 3 : (rssi >= -75) ? 2 : (rssi >= -85) ? 1 : 0;
    for (int i = 0; i < 4; i++) {
        int h = 3 * (i + 1);
        g.fillRect(x + i * 4, 14 - h, 3, h, (level > i) ? C_TEXT : C_HEAD_DK);
    }
}

// Right side of the header, anchored to the right edge: [ago] [wifi] [battery+pct].
// Repainted whole by uiDashboardClock every 10s, so everything here must be
// derivable from its arguments.
static void drawHeaderRight(TFT_eSPI& g, int rssi, unsigned long ago, int batPct) {
    g.setTextColor(C_TEXT, C_HEAD);
    g.setTextSize(1);

    char ps[8];
    snprintf(ps, sizeof(ps), "%d%%", batPct);
    int x = SCREEN_W - 4 - (int)strlen(ps) * 6;
    g.setCursor(x, 5);
    g.print(ps);

    x -= 24;   // battery: 18 body + 2 nub + 4 gap before the text
    g.drawRect(x, 4, 18, 10, C_TEXT);
    g.fillRect(x + 18, 7, 2, 4, C_TEXT);
    int fw = 14 * constrain(batPct, 0, 100) / 100;
    if (fw > 0) g.fillRect(x + 2, 6, fw, 6, C_TEXT);

    x -= 21;   // wifi: 15 wide + 6 gap
    drawWifiIcon(g, x, rssi);

    char as[12];
    snprintf(as, sizeof(as), "%lus", ago);
    g.setCursor(x - 6 - (int)strlen(as) * 6, 5);
    g.print(as);
}
#endif // MANGO_UI

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

#ifdef MANGO_UI
    lcd.setCursor(SCREEN_W - SX(4) - (int)strlen("v" FW_VERSION) * TS(6), SCREEN_H - SY(12));
    lcd.print("v" FW_VERSION);
#endif
    halFlush();
}

void uiSetupScreen(const char* apName, const char* apPass, bool reconfigure) {
    halClear(C_BG);

    lcd.fillRect(0, 0, SCREEN_W, SY(18), C_ACCENT);
    lcd.setTextColor(C_TEXT, C_ACCENT);
    lcd.setTextSize(TS(1));
    lcd.setCursor(SX(6), SY(5));
    lcd.print(reconfigure ? "WIFI RECONNECT" : "SETUP MODE");

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

    if (reconfigure) {
        lcd.setTextColor(C_DIM, C_BG);
        lcd.setTextSize(TS(1));
        lcd.setCursor(SX(10), SY(124));
        lcd.print("Token, PIN & settings kept");
    }
    halFlush();
}

void uiPinScreen(int pos, const int digits[4]) {
#ifdef BOARD_CROWPANEL_ADV_35
    auto& g = dashTarget();
    g.fillSprite(C_BG);
#else
    auto& g = lcd;
    halClear(C_BG);
#endif

    int boxW = SX(30), boxH = SY(36), gap = SX(12);
    int startX = (SCREEN_W - (4 * boxW + 3 * gap)) / 2;
#ifdef MANGO_UI
    int boxY = (SCREEN_H - boxH) / 2;   // dead-center of the screen
#else
    int boxY = SY(40);
#endif

#ifdef MANGO_UI
    // Same dress code as the dashboard: orange header band + Clawd standing guard.
    // Label/hint/note positions hang off boxY so the block stays centered on both
    // the 320x170 (S3) and 240x135 (M5StickC Plus) panels.
    g.fillRect(0, 0, SCREEN_W, 18, C_HEAD);
    g.setTextColor(C_TEXT, C_HEAD);
    g.setTextSize(1);
    g.setCursor(4, 5);
    g.print("CLAUDE USAGE");
    g.setCursor(SCREEN_W - 4 - 6 * 6, 5);
    g.print("LOCKED");
    g.setTextColor(C_DIM, C_BG);
    g.setCursor((SCREEN_W - 10 * 6) / 2, boxY - 18);
    g.print("UNLOCK PIN");
#else
    g.setTextColor(C_DIM, C_BG);
    g.setTextSize(TS(1));
    g.setCursor(SX(70), SY(15));
    g.print("UNLOCK PIN");
#endif

    for (int i = 0; i < 4; i++) {
        int x = startX + i * (boxW + gap);
        uint16_t borderCol = (i == pos) ? C_CYAN : C_DIM;

        g.drawRect(x, boxY, boxW, boxH, borderCol);
        if (i == pos) g.drawRect(x + 1, boxY + 1, boxW - 2, boxH - 2, borderCol);

        g.setTextSize(TS(3));
        if (i < pos) {
            g.setTextColor(C_ACCENT, C_BG);
            g.setCursor(x + SX(9), boxY + SY(7));
            g.print("*");
        } else if (i == pos) {
            g.setTextColor(C_TEXT, C_BG);
            g.setCursor(x + SX(9), boxY + SY(7));
            g.print(digits[i]);
        }
    }

#ifdef MANGO_UI
    const int hintY = boxY + boxH + 12;   // just below the centered boxes
#else
    const int hintY = SY(95);
#endif
    g.setTextColor(C_DIM, C_BG);
    g.setTextSize(TS(1));
    g.setCursor(SX(20), hintY);
#ifdef BOARD_T8_S2
    // Single BOOT button: short tap cycles the digit, long press confirms.
    g.print("[tap] cycle digit");
    g.setCursor(SX(148), SY(95));
    g.print("[hold] confirm");

    g.setCursor(SX(35), SY(118));
    g.setTextColor(0x4A49, C_BG);
    g.print("short tap = A    long press = B");
    halFlush();
#elif defined(BOARD_CROWPANEL_ADV_35)
    // Touch HMI — stack the hints vertically; the side-by-side layout overflows at this scale.
    g.print("tap LEFT  = next digit");
    g.setCursor(SX(20), SY(110));
    g.print("tap RIGHT = confirm");
    g.setCursor(SX(20), SY(124));
    g.setTextColor(0x4A49, C_BG);
    g.print("re-flash to factory reset");

    // Push the full frame on entry (all digits empty at pos 0); afterwards push only the
    // digit-box band — a full-width region is contiguous in the sprite buffer, so each tap
    // updates quickly and without the flicker a direct clear-and-redraw would cause.
    if (pos == 0 && digits[0] == 0 && digits[1] == 0 && digits[2] == 0 && digits[3] == 0) {
        s_dash.pushSprite(0, 0);
    } else {
        int bandY = SY(38), bandH = SY(42);
        lcd.pushImage(0, bandY, SCREEN_W, bandH,
                      (uint16_t*)s_dash.getPointer() + (size_t)bandY * SCREEN_W);
    }
#else
#ifdef MANGO_UI
    static const char* hint = "[A] cycle digit   [B] confirm";
    g.setCursor((SCREEN_W - (int)strlen(hint) * 6) / 2, hintY);
    g.print(hint);

    static const char* note = "Hold A+B on boot = factory reset";
    g.setTextColor(0x4A49, C_BG);
    g.setCursor((SCREEN_W - (int)strlen(note) * 6) / 2, boxY + boxH + 32);
    g.print(note);

#ifdef BOARD_TDISPLAY_S3
    // WiFi is already up on this board, so the device's LAN address is known
    // while it sits locked.
    if (s_netUrl[0]) {
        g.setTextColor(C_CYAN, C_BG);
        g.setCursor((SCREEN_W - (int)strlen(s_netUrl) * 6) / 2, boxY + boxH + 48);
        g.print(s_netUrl);
    }
#endif
#else
    g.print("[A] cycle digit");
    g.setCursor(SX(148), hintY);
    g.print("[B] confirm");

    g.setCursor(SX(35), SY(118));
    g.setTextColor(0x4A49, C_BG);
    g.print("Hold A+B on boot = factory reset");
#endif
    halFlush();
#endif
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
#ifdef BOARD_CROWPANEL_ADV_35
    auto& g = dashTarget();
    g.fillSprite(C_BG);
#else
    auto& g = lcd;   // deduces the board's real surface type (panel, sprite, or M5 LCD)
    halClear(C_BG);
#endif

    // Header
    g.fillRect(0, 0, SCREEN_W, SY(18), C_HEAD);
#ifdef BOARD_TDISPLAY_S3
    drawHeaderLeft(g);
#else
    g.setTextColor(C_TEXT, C_HEAD);
    g.setTextSize(TS(1));
    g.setCursor(SX(4), SY(5));
    g.print("CLAUDE USAGE");
#endif

    unsigned long ago = (millis() - lastFetchMs) / 1000;
#ifdef MANGO_UI
    drawHeaderRight(g, rssi, ago, batPct);
#else
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "%ddBm %lus", rssi, ago);
    g.setCursor(SCREEN_W - strlen(hdr) * TS(6) - SX(4), SY(5));
    g.print(hdr);
#endif

    if (!data.ok) {
        g.setTextColor(C_CRIT, C_BG);
        g.setTextSize(TS(2));
        g.setCursor(SX(10), SY(35));
        g.print("ERROR");
        g.setTextSize(TS(1));
        g.setTextColor(C_DIM, C_BG);
        g.setCursor(SX(10), SY(60));
        g.print(data.error);
        g.setCursor(SX(10), SY(80));
#ifdef MANGO_UI
        g.print("retrying automatically...");   // B is brightness on this board
#else
        g.print("[B] retry now");
#endif
        UI_PUSH_DASH();
        return;
    }

    int barW = SCREEN_W - SX(20);

    char h5rst[16], d7rst[16];
    fmtCountdown(data.h5ResetEpoch, h5rst, sizeof(h5rst));
    fmtCountdown(data.d7ResetEpoch, d7rst, sizeof(d7rst));

#ifdef MANGO_UI
#ifdef BOARD_TDISPLAY_S3
    // Tier L: % flush-right on the bar rows; the countdowns get their own
    // size-2 row below the bars.
    drawBar(g, SX(10), SY(24), barW, SY(10), data.h5, "5-HOUR");
    drawBar(g, SX(10), SY(52), barW, SY(10), data.d7, "7-DAY");
    drawResetRow(g, h5rst, d7rst);
#else
    // Tier S: each reset countdown rides on its bar's label row — no room below.
    drawBar(g, SX(10), SY(24), barW, SY(10), data.h5, "5-HOUR", h5rst);
    drawBar(g, SX(10), SY(52), barW, SY(10), data.d7, "7-DAY",  d7rst);
#endif
    drawStatusPanel(g);
#else
    drawBar(g, SX(10), SY(24), barW, SY(10), data.h5, "5-HOUR WINDOW");
    drawBar(g, SX(10), SY(52), barW, SY(10), data.d7, "7-DAY WINDOW");

    g.setTextColor(C_DIM, C_BG);
    g.setTextSize(TS(1));
    g.setCursor(SX(10), SY(80));
    g.print("5H RST");
    g.setTextColor(C_TEXT, C_BG);
    g.setTextSize(TS(2));
    g.setCursor(SX(10), SY(92));
    g.printf("%-8s", h5rst);

    g.setTextColor(C_DIM, C_BG);
    g.setTextSize(TS(1));
    g.setCursor(SCREEN_W / 2 + SX(10), SY(80));
    g.print("7D RST");
    g.setTextColor(C_TEXT, C_BG);
    g.setTextSize(TS(2));
    g.setCursor(SCREEN_W / 2 + SX(10), SY(92));
    g.printf("%-8s", d7rst);

    g.setTextColor(C_DIM, C_BG);
    g.setTextSize(TS(1));
    g.setCursor(SCREEN_W - SX(48), SY(120));
    g.printf("BAT %d%%", batPct);
#endif
    UI_PUSH_DASH();
}

void uiDashboardClock(const UsageData& data, unsigned long lastFetchMs, int rssi) {
    if (!data.ok) return;   // error layout is owned by the full uiDashboard
#ifdef BOARD_CROWPANEL_ADV_35
    auto& g = dashTarget();   // update the retained sprite, then push it once
#else
    auto& g = lcd;
#endif

    // Header "rssi / ago": repaint the header band over its own colour.
    unsigned long ago = (millis() - lastFetchMs) / 1000;
#ifdef BOARD_TDISPLAY_S3
    // Ride the 10s tick to alternate the left text (device label ↔ panel URL).
    s_hdrShowUrl = !s_hdrShowUrl;
    drawHeaderLeft(g);
#endif
    g.fillRect(SCREEN_W / 2, 0, SCREEN_W / 2, SY(18), C_HEAD);
    g.setTextColor(C_TEXT, C_HEAD);
    g.setTextSize(TS(1));
#ifdef MANGO_UI
    drawHeaderRight(g, rssi, ago, halBatPercent());
#else
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "%ddBm %lus", rssi, ago);
    g.setCursor(SCREEN_W - strlen(hdr) * TS(6) - SX(4), SY(5));
    g.print(hdr);
#endif

    // Reset countdowns: repaint in place.
    char h5rst[16], d7rst[16];
    fmtCountdown(data.h5ResetEpoch, h5rst, sizeof(h5rst));
    fmtCountdown(data.d7ResetEpoch, d7rst, sizeof(d7rst));
#ifdef MANGO_UI
#ifdef BOARD_TDISPLAY_S3
    // Tier L: the countdowns live on their own row below the bars.
    drawResetValues(g, h5rst, d7rst);
#else
    // Tier S: the countdowns live on the bar rows; refresh just those slots.
    int barW = SCREEN_W - SX(20);
    drawResetSlot(g, SX(10), barW, SY(24), h5rst);
    drawResetSlot(g, SX(10), barW, SY(52), d7rst);
#endif
#else
    g.setTextColor(C_TEXT, C_BG);
    g.setTextSize(TS(2));
    g.setCursor(SX(10), SY(92));
    g.printf("%-8s", h5rst);
    g.setCursor(SCREEN_W / 2 + SX(10), SY(92));
    g.printf("%-8s", d7rst);
#endif

    UI_PUSH_DASH();
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

void uiLockoutStatic(int attempts, int maxAttempts, int lockoutSec) {
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
}

void uiLockoutTick(int secondsLeft) {
    lcd.fillRect(SX(10), SY(95), SX(200), SY(20), C_BG);
    lcd.setTextColor(C_WARN, C_BG);
    lcd.setTextSize(TS(2));
    lcd.setCursor(SX(10), SY(95));
    lcd.printf("%ds", secondsLeft);
    halFlush();
}

#endif // BOARD_ESP32C3_OLED
