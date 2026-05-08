#pragma once
#include <stdint.h>

#ifdef BOARD_ESP32C3_OLED
  #include <U8g2lib.h>
  #include <Wire.h>
  // SSD1306 128×64 controller; 72×40 pixels physically wired. Use NONAME init
  // sequence — the ER variant has a different command set that silently fails.
  extern U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2;
#elif defined(BOARD_TDISPLAY_S3)
  #include <TFT_eSPI.h>
  extern TFT_eSPI lcd;
#elif defined(BOARD_M5STICK_C_PLUS2)
  #include <M5Unified.h>
  #define lcd M5.Display
#else
  #include <M5StickCPlus.h>
  #ifdef lcd
    #undef lcd
  #endif
  #define lcd M5.Lcd
#endif

void halInit();
void halUpdate();
bool halBtnAWasPressed();
bool halBtnBWasPressed();
bool halBtnAIsPressed();
bool halBtnBIsPressed();
int  halBatPercent();
void halSetBrightness(uint8_t level);
