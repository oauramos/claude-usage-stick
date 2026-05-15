#pragma once
#include <stdint.h>

#ifdef BOARD_TDISPLAY_S3
  #include <TFT_eSPI.h>
  extern TFT_eSPI lcd;
#elif defined(BOARD_TDISPLAY_S3_AMOLED)
  #include <LilyGo_AMOLED.h>
  #include <TFT_eSPI.h>
  extern LilyGo_Class amoled;
  extern TFT_eSprite  spr;
  #define lcd spr
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
void halFlush();
void halClear(uint16_t color);
