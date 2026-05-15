#include "hal.h"
#include <Arduino.h>

#ifdef BOARD_TDISPLAY_S3

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
