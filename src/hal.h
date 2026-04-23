#pragma once
#include <stdint.h>

#ifdef BOARD_TDISPLAY_S3
  #include <TFT_eSPI.h>
  extern TFT_eSPI lcd;
#else
  #include <M5StickCPlus.h>
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
