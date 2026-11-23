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
#elif defined(BOARD_WAVESHARE_C6_LCD)
  // LovyanGFX with a TFT_eSPI-compatible surface (ui.cpp only names TFT_eSprite).
  #define LGFX_USE_V1
  #include <LovyanGFX.hpp>

  class LGFX_WaveshareC6 : public lgfx::LGFX_Device {
      lgfx::Panel_ST7789 _panel;
      lgfx::Bus_SPI      _bus;
      lgfx::Light_PWM    _light;
  public:
      LGFX_WaveshareC6() {
          {
              auto cfg = _bus.config();
              cfg.spi_host    = SPI2_HOST;
              cfg.spi_mode    = 0;
              cfg.freq_write  = 40000000;
              cfg.freq_read   = 16000000;
              cfg.spi_3wire   = false;
              cfg.use_lock    = true;
              cfg.dma_channel = SPI_DMA_CH_AUTO;
              cfg.pin_sclk    = 7;
              cfg.pin_mosi    = 6;
              cfg.pin_miso    = -1;
              cfg.pin_dc      = 15;
              _bus.config(cfg);
              _panel.setBus(&_bus);
          }
          {
              // 172 visible columns centred in the controller's 240-wide RAM.
              auto cfg = _panel.config();
              cfg.pin_cs        = 14;
              cfg.pin_rst       = 21;
              cfg.pin_busy      = -1;
              cfg.memory_width  = 240;
              cfg.memory_height = 320;
              cfg.panel_width   = 172;
              cfg.panel_height  = 320;
              cfg.offset_x      = 34;
              cfg.offset_y      = 0;
              cfg.readable      = false;
              cfg.invert        = true;   // IPS panel
              cfg.rgb_order     = false;
              cfg.bus_shared    = false;
              _panel.config(cfg);
          }
          {
              auto cfg = _light.config();
              cfg.pin_bl      = 22;
              cfg.invert      = false;
              cfg.freq        = 5000;
              cfg.pwm_channel = 0;
              _light.config(cfg);
              _panel.setLight(&_light);
          }
          setPanel(&_panel);
      }
  };

  using TFT_eSprite = LGFX_Sprite;
  extern LGFX_WaveshareC6 lcd;
#elif defined(BOARD_TDISPLAY_ESP32)
  #include <TFT_eSPI.h>
  extern TFT_eSPI lcd;
#elif defined(BOARD_T8_S2)
  #include <TFT_eSPI.h>
  extern TFT_eSPI lcd;
#elif defined(BOARD_CROWPANEL_ADV_35)
  #include <TFT_eSPI.h>
  #include <Wire.h>
  #include <TAMC_GT911.h>
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
