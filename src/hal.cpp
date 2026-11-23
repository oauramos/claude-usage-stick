#include "hal.h"
#include <Arduino.h>

#ifdef BOARD_ESP32C3_OLED

// Button A: GPIO 3 (external button, active-HIGH — connect to 3.3 V when pressed)
// Button B: GPIO 7 (external button, active-HIGH — connect to 3.3 V when pressed)
// GPIO 9 (BO0) is a strapping pin — left for download mode only.
#define BTN_A_PIN  3
#define BTN_B_PIN  7
#define LED_PIN    8   // Onboard blue LED
#define SDA_PIN    5
#define SCL_PIN    6

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL_PIN, SDA_PIN);

static bool prevA = false, prevB = false;
static bool tapA  = false, tapB  = false;

void halInit() {
    // u8g2.begin() owns Wire init — do not call Wire.begin() first,
    // as double-init locks up the ESP32 I2C driver.
    u8g2.begin();
    u8g2.setBusClock(400000);
    u8g2.setContrast(255);

    // Buttons are active-HIGH: pressed = HIGH, idle = LOW → use PULLDOWN
    pinMode(BTN_A_PIN, INPUT_PULLDOWN);
    pinMode(BTN_B_PIN, INPUT_PULLDOWN);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // active-LOW LED: HIGH = off
}

void halUpdate() {
    bool a = digitalRead(BTN_A_PIN);
    bool b = digitalRead(BTN_B_PIN);
    tapA = (a && !prevA);
    tapB = (b && !prevB);
    prevA = a;
    prevB = b;
}

bool halBtnAWasPressed() { bool r = tapA; tapA = false; return r; }
bool halBtnBWasPressed() { bool r = tapB; tapB = false; return r; }
bool halBtnAIsPressed()  { return prevA; }
bool halBtnBIsPressed()  { return prevB; }

int halBatPercent() { return -1; } // No battery monitoring on this board

void halSetBrightness(uint8_t level) {
    // SSD1306 contrast change is imperceptible on this panel — just on/off
    if (level == 0) {
        u8g2.setPowerSave(1);
    } else {
        u8g2.setPowerSave(0);
        u8g2.setContrast(255);
    }
}

#elif defined(BOARD_TDISPLAY_S3)

TFT_eSPI lcd;

#define BTN_A_PIN  0
#define BTN_B_PIN  14
#define BAT_ADC    4
#define BL_PIN     38
#define PWR_EN     15

static bool prevA = false, prevB = false;
static bool tapA = false, tapB = false;

void halInit() {
    pinMode(PWR_EN, OUTPUT);
    digitalWrite(PWR_EN, HIGH);
    lcd.init();
    lcd.invertDisplay(true);
    pinMode(BTN_A_PIN, INPUT_PULLUP);
    pinMode(BTN_B_PIN, INPUT_PULLUP);
    ledcSetup(0, 5000, 8);
    ledcAttachPin(BL_PIN, 0);
    ledcWrite(0, 200);
}

void halUpdate() {
    bool a = !digitalRead(BTN_A_PIN);
    bool b = !digitalRead(BTN_B_PIN);
    tapA = (a && !prevA);
    tapB = (b && !prevB);
    prevA = a;
    prevB = b;
}

bool halBtnAWasPressed() { bool r = tapA; tapA = false; return r; }
bool halBtnBWasPressed() { bool r = tapB; tapB = false; return r; }
bool halBtnAIsPressed()  { return !digitalRead(BTN_A_PIN); }
bool halBtnBIsPressed()  { return !digitalRead(BTN_B_PIN); }

int halBatPercent() {
    uint16_t raw = analogRead(BAT_ADC);
    float v = (raw / 4095.0f) * 3.3f * 2.0f;
    return constrain((int)((v - 3.3f) / 0.85f * 100), 0, 100);
}

void halSetBrightness(uint8_t level) {
    static const uint8_t vals[] = {0, 60, 160, 255};
    ledcWrite(0, vals[level]);
}

void halFlush() {}

void halClear(uint16_t color) { lcd.fillScreen(color); }

#elif defined(BOARD_WAVESHARE_C6_LCD)

// Waveshare ESP32-C6-LCD-1.47 (ST7789 172x320 SPI, no PSRAM, no battery).
// The only user button is BOOT (GPIO9, active-LOW), so the two logical
// buttons come from one: a tap = A, a hold of BTN_LONG_MS = B (fires while
// still held, so the user feels it land). Both are momentary edges, so
// halBtn*IsPressed() stay false and the A+B gestures (boot factory reset,
// force refresh) are unreachable; the web panel covers both.
// Panel, bus and backlight are configured in hal.h (LovyanGFX).
LGFX_WaveshareC6 lcd;

#define BTN_PIN      9
#define BTN_LONG_MS  700
#define BTN_MIN_MS   30    // debounce: shorter blips are ignored

static bool btnDown = false, longFired = false;
static unsigned long downAt = 0;
static bool tapA = false, tapB = false;

void halInit() {
    lcd.init();   // inversion comes from the panel config
    lcd.setBrightness(200);
    pinMode(BTN_PIN, INPUT_PULLUP);
}

void halUpdate() {
    // Edges last one update, like the two-button boards.
    tapA = tapB = false;
    bool pressed = !digitalRead(BTN_PIN);
    unsigned long now = millis();
    if (pressed && !btnDown) {
        btnDown = true;
        longFired = false;
        downAt = now;
    } else if (pressed && !longFired && now - downAt >= BTN_LONG_MS) {
        longFired = true;
        tapB = true;
    } else if (!pressed && btnDown) {
        btnDown = false;
        if (!longFired && now - downAt >= BTN_MIN_MS) tapA = true;
    }
}

bool halBtnAWasPressed() { bool r = tapA; tapA = false; return r; }
bool halBtnBWasPressed() { bool r = tapB; tapB = false; return r; }
bool halBtnAIsPressed()  { return false; }
bool halBtnBIsPressed()  { return false; }

int halBatPercent() { return -1; }  // USB powered, no battery sense

void halSetBrightness(uint8_t level) {
    static const uint8_t vals[] = {0, 60, 160, 255};
    lcd.setBrightness(vals[level]);
}

void halFlush() {}

void halClear(uint16_t color) { lcd.fillScreen(color); }

#elif defined(BOARD_TDISPLAY_ESP32)

// TTGO/LilyGo T-Display ESP32 (1.14" ST7789 135x240, 4-wire SPI).
TFT_eSPI lcd;

#define BTN_A_PIN  0    // also BOOT button; onboard pull-up, active-LOW
#define BTN_B_PIN  35   // input-only pin (no internal pull); onboard pull-up, active-LOW
#define BAT_ADC    34   // battery voltage via 2:1 divider
#define ADC_EN     14   // drive HIGH to enable the battery divider
#define BL_PIN     4

static bool prevA = false, prevB = false;
static bool tapA = false, tapB = false;

void halInit() {
    pinMode(ADC_EN, OUTPUT);
    digitalWrite(ADC_EN, HIGH);
    lcd.init();
    pinMode(BTN_A_PIN, INPUT_PULLUP);
    pinMode(BTN_B_PIN, INPUT);       // GPIO35 is input-only; relies on board pull-up
    ledcSetup(0, 5000, 8);
    ledcAttachPin(BL_PIN, 0);
    ledcWrite(0, 200);
}

void halUpdate() {
    bool a = !digitalRead(BTN_A_PIN);
    bool b = !digitalRead(BTN_B_PIN);
    tapA = (a && !prevA);
    tapB = (b && !prevB);
    prevA = a;
    prevB = b;
}

bool halBtnAWasPressed() { bool r = tapA; tapA = false; return r; }
bool halBtnBWasPressed() { bool r = tapB; tapB = false; return r; }
bool halBtnAIsPressed()  { return !digitalRead(BTN_A_PIN); }
bool halBtnBIsPressed()  { return !digitalRead(BTN_B_PIN); }

int halBatPercent() {
    uint16_t raw = analogRead(BAT_ADC);
    float v = (raw / 4095.0f) * 3.3f * 2.0f;
    return constrain((int)((v - 3.3f) / 0.85f * 100), 0, 100);
}

void halSetBrightness(uint8_t level) {
    static const uint8_t vals[] = {0, 60, 160, 255};
    ledcWrite(0, vals[level]);
}

void halFlush() {}

void halClear(uint16_t color) { lcd.fillScreen(color); }

#elif defined(BOARD_T8_S2)

// LilyGo T8 ESP32-S2 (1.14" ST7789 135x240, 4-wire SPI on the FSPI bus). Same panel
// as the TTGO T-Display, so all drawing is identical — only the pin mapping differs.
//
// The board exposes only the onboard BOOT button (GPIO0) as a usable input, so the
// two-button UX is folded onto a single button by press duration:
//   short tap  -> Button A (cycle digit / cycle brightness)
//   long press -> Button B (confirm digit / force refresh)
// GPIO0 is a strapping pin — holding it during reset enters download mode — so the
// "hold A+B on boot" factory reset is unavailable here; re-flash to wipe NVS.
TFT_eSPI lcd;

#define BTN_PIN       0     // onboard BOOT button; active-LOW via onboard pull-up
#define BL_PIN        33    // matches the TFT_BL build flag (verified on hardware)
#define LONGPRESS_MS  600   // hold at least this long to register as Button B

static bool     prevDown  = false;
static uint32_t pressedAt = 0;
static bool     longFired = false;
static bool     tapA = false, tapB = false;

void halInit() {
    lcd.init();
    pinMode(BTN_PIN, INPUT_PULLUP);
    ledcSetup(0, 5000, 8);
    ledcAttachPin(BL_PIN, 0);
    ledcWrite(0, 200);
}

void halUpdate() {
    bool down = !digitalRead(BTN_PIN);   // active-LOW
    uint32_t now = millis();
    if (down && !prevDown) {                              // press begins
        pressedAt = now;
        longFired = false;
    } else if (down && prevDown) {                        // held down
        if (!longFired && now - pressedAt >= LONGPRESS_MS) {
            tapB = true;                                  // long press -> B (fires once)
            longFired = true;
        }
    } else if (!down && prevDown) {                       // released
        if (!longFired && now - pressedAt < LONGPRESS_MS) {
            tapA = true;                                  // short tap -> A
        }
    }
    prevDown = down;
}

bool halBtnAWasPressed() { bool r = tapA; tapA = false; return r; }
bool halBtnBWasPressed() { bool r = tapB; tapB = false; return r; }
bool halBtnAIsPressed()  { return !digitalRead(BTN_PIN); }
bool halBtnBIsPressed()  { return false; } // no distinct "B held" with a single button

int halBatPercent() { return -1; } // TODO(hardware): no confirmed battery-sense ADC on T8-S2

void halSetBrightness(uint8_t level) {
    static const uint8_t vals[] = {0, 60, 160, 255};
    ledcWrite(0, vals[level]);
}

void halFlush() {}

void halClear(uint16_t color) { lcd.fillScreen(color); }

#elif defined(BOARD_TDISPLAY_S3_AMOLED)

// LilyGo T-Display S3 AMOLED (1.91" RM67162) — H712/H713/H705/H681/H717.
// Single physical button (GPIO0). Button B is mapped to a touch tap when the
// touch controller is present (H705/H681/H717), and is otherwise unavailable.

LilyGo_Class amoled;
static TFT_eSPI _tftParent;
TFT_eSprite    spr = TFT_eSprite(&_tftParent);

#define BTN_A_PIN 0

static bool prevA = false, prevB = false;
static bool tapA = false, tapB = false;
static bool hasTouch = false;

void halInit() {
    if (!amoled.begin()) {
        Serial.println("AMOLED begin() failed");
    }
    amoled.setRotation(0);                  // landscape: 536 wide, 240 tall
    spr.setColorDepth(16);
    spr.createSprite(amoled.width(), amoled.height());
    spr.setSwapBytes(true);
    spr.fillSprite(TFT_BLACK);
    amoled.pushColors(0, 0, amoled.width(), amoled.height(),
                      (uint16_t *)spr.getPointer());
    amoled.setBrightness(200);
    hasTouch = amoled.hasTouch();
    pinMode(BTN_A_PIN, INPUT_PULLUP);
}

void halUpdate() {
    bool a = !digitalRead(BTN_A_PIN);
    bool b = false;
    if (hasTouch) {
        b = amoled.isPressed();
    }
    tapA = (a && !prevA);
    tapB = (b && !prevB);
    prevA = a;
    prevB = b;
}

bool halBtnAWasPressed() { bool r = tapA; tapA = false; return r; }
bool halBtnBWasPressed() { bool r = tapB; tapB = false; return r; }
bool halBtnAIsPressed()  { return !digitalRead(BTN_A_PIN); }
bool halBtnBIsPressed()  { return hasTouch && amoled.isPressed(); }

int halBatPercent() {
    uint16_t mv = amoled.getBattVoltage();
    if (mv == 0) return 100;                // USB only, no battery
    float v = mv / 1000.0f;
    return constrain((int)((v - 3.3f) / 0.85f * 100), 0, 100);
}

void halSetBrightness(uint8_t level) {
    static const uint8_t vals[] = {0, 60, 160, 255};
    amoled.setBrightness(vals[level]);
}

void halFlush() {
    amoled.pushColors(0, 0, amoled.width(), amoled.height(),
                      (uint16_t *)spr.getPointer());
}

void halClear(uint16_t color) {
    uint32_t pixels = (uint32_t)amoled.width() * amoled.height();
    uint16_t *buf = (uint16_t *)spr.getPointer();
    if (!buf) return;
    for (uint32_t i = 0; i < pixels; i++) buf[i] = color;
}

#elif defined(BOARD_CROWPANEL_ADV_35)

// Elecrow CrowPanel Advance 3.5" HMI (ESP32-S3, 480x320 IPS, ILI9488 over SPI).
// It has no physical user buttons — input is the GT911 capacitive touch panel,
// split into two zones: a tap on the LEFT half acts as Button A (cycle digit /
// brightness), the RIGHT half as Button B (confirm / refresh). This matches the
// on-screen "tap left / tap right" legend positions. The A+B factory-reset combo needs
// two buttons and is unavailable here (a single touch point is read); re-flash to wipe NVS.
//
// lcd is the panel itself. The dashboard renders into a PSRAM sprite and is pushed in one
// transfer so refreshes don't flicker on this slow SPI panel (see ui.cpp); the PIN/setup
// screens draw straight to the panel so touch input stays responsive.
TFT_eSPI lcd;

#define BL_PIN     38
#define TOUCH_SDA  15
#define TOUCH_SCL  16
#define CP_W       480   // landscape width  (mirrors SCREEN_W in config.h)
#define CP_H       320   // landscape height (mirrors SCREEN_H in config.h)

// GT911 over I2C; INT/RST are not broken out on this board (-1).
TAMC_GT911 touch(TOUCH_SDA, TOUCH_SCL, -1, -1, CP_W, CP_H);

static bool prevTouch = false;
static bool tapA = false, tapB = false;
static bool downA = false, downB = false;

void halInit() {
    lcd.init();
    lcd.invertDisplay(true);     // ILI9488 panel runs inverted (per Elecrow's driver)
    ledcSetup(0, 5000, 8);
    ledcAttachPin(BL_PIN, 0);
    ledcWrite(0, 200);

    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    touch.begin();
    // Read raw GT911 coordinates and map zones manually — the library's rotation
    // transforms underflow when the panel's native axes don't match width/height.
    touch.setRotation(ROTATION_INVERTED);
}

void halUpdate() {
    touch.read();
    bool down = touch.isTouched;
    if (down && !prevTouch) {                          // new finger press (rising edge)
        // Raw GT911 axes: points[].y is the horizontal screen axis (left ~479 .. right ~0,
        // inverted); points[].x is vertical. Split on the horizontal midpoint.
        if (touch.points[0].y > (CP_W / 2)) { tapA = true; downA = true; downB = false; }  // left half
        else                                { tapB = true; downB = true; downA = false; }  // right half
    }
    if (!down) { downA = false; downB = false; }
    prevTouch = down;
}

bool halBtnAWasPressed() { bool r = tapA; tapA = false; return r; }
bool halBtnBWasPressed() { bool r = tapB; tapB = false; return r; }
bool halBtnAIsPressed()  { return downA; }
bool halBtnBIsPressed()  { return downB; }

int halBatPercent() { return -1; }  // no battery-sense ADC on this board

void halSetBrightness(uint8_t level) {
    static const uint8_t vals[] = {0, 60, 160, 255};
    ledcWrite(0, vals[level]);
}

void halFlush() {}

void halClear(uint16_t color) { lcd.fillScreen(color); }

#elif defined(BOARD_M5STICK_C_PLUS2)

#define HOLD_PIN 4

void halInit() {
    pinMode(HOLD_PIN, OUTPUT);
    digitalWrite(HOLD_PIN, HIGH);

    auto cfg = M5.config();
    M5.begin(cfg);
}

void halUpdate() {
    M5.update();
}

bool halBtnAWasPressed() { return M5.BtnA.wasPressed(); }
bool halBtnBWasPressed() { return M5.BtnB.wasPressed(); }
bool halBtnAIsPressed()  { return M5.BtnA.isPressed(); }
bool halBtnBIsPressed()  { return M5.BtnB.isPressed(); }

int halBatPercent() {
    int pct = M5.Power.getBatteryLevel();
    if (pct >= 0 && pct <= 100) {
        return pct;
    }

    const int mv = M5.Power.getBatteryVoltage();
    const float v = mv / 1000.0f;
    return constrain((int)((v - 3.3f) / 0.85f * 100), 0, 100);
}

void halSetBrightness(uint8_t level) {
    static const uint8_t vals[] = {0, 64, 160, 255};
    M5.Display.setBrightness(vals[level]);
}

void halFlush() {}

void halClear(uint16_t color) { lcd.fillScreen(color); }

#else // M5StickC Plus

void halInit() {
    M5.begin();
}

void halUpdate() {
    M5.update();
}

bool halBtnAWasPressed() { return M5.BtnA.wasPressed(); }
bool halBtnBWasPressed() { return M5.BtnB.wasPressed(); }
bool halBtnAIsPressed()  { return M5.BtnA.isPressed(); }
bool halBtnBIsPressed()  { return M5.BtnB.isPressed(); }

int halBatPercent() {
    float v = M5.Axp.GetBatVoltage();
    return constrain((int)((v - 3.3f) / 0.85f * 100), 0, 100);
}

void halSetBrightness(uint8_t level) {
    static const uint8_t vals[] = {0, 30, 80, 160};
    M5.Axp.ScreenBreath(vals[level]);
}

void halFlush() {}

void halClear(uint16_t color) { lcd.fillScreen(color); }

#endif
