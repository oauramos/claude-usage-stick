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

#elif defined(BOARD_GEEKMAGIC_SMALLTV)

// GeekMagic SmallTV / GIF.TV (ESP8266 ESP-12F, 1.5" 240x240 ST7789 on the HSPI bus).
// No buttons and no battery: the button HAL is stubbed (the boot PIN is entered over
// the web instead — see runWebUnlock), and battery reads return -1. Backlight is PWM'd
// with analogWrite() since the ESP8266 has no LEDC peripheral. Pins come from the
// hardware reference doc — VERIFY against your PCB revision with a multimeter.
TFT_eSPI lcd;

#define BL_PIN 2   // GPIO2 PWM backlight (also a boot strapping pin — must be HIGH at boot)

void halInit() {
    lcd.init();
    // Rotation is applied by uiInit() (SCREEN_ROT); color inversion comes from the
    // TFT_INVERSION_ON build flag (stock firmware uses INVON).
    pinMode(BL_PIN, OUTPUT);
    analogWriteRange(255);
    analogWrite(BL_PIN, 200);
}

void halUpdate() {}   // no buttons to sample

bool halBtnAWasPressed() { return false; }
bool halBtnBWasPressed() { return false; }
bool halBtnAIsPressed()  { return false; }
bool halBtnBIsPressed()  { return false; }

int halBatPercent() { return -1; }   // USB-powered, no battery sense

void halSetBrightness(uint8_t level) {
    static const uint8_t vals[] = {0, 60, 160, 255};
    analogWrite(BL_PIN, vals[level]);
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
