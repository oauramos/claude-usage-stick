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

#endif
