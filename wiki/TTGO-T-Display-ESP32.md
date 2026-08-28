# TTGO T-Display ESP32

Part of [Claude Usage Stick](Home). The classic LilyGo/TTGO T-Display — an original ESP32 with a 1.14" ST7789 panel over 4-wire SPI and two front buttons.

## Specs

| | |
| --- | --- |
| MCU | ESP32 (dual-core, WiFi) |
| Display | 1.14" ST7789 LCD, 135×240 (SPI: MOSI 19, SCLK 18, CS 5, DC 16, RST 23, BL 4) |
| Battery | External via JST 1.25 mm connector |
| Buttons | Button A (BOOT, GPIO 0) · Button B (GPIO 35, input-only pin with onboard pull-up) |
| Firmware | Clarity (v1) — target display tier **S** (not yet migrated) |
| PlatformIO env | `tdisplay-esp32` |
| Buy | [aliexpress.com](https://s.click.aliexpress.com/e/_c32HlGQ1) |

## Flash

Easiest: open the **[web flasher](https://oauramos.github.io/claude-usage-stick/)** in Chrome or Edge, pick this board, and plug it in over USB-C.

From source, with the [PlatformIO CLI](https://platformio.org/install/cli):

```bash
pio run -e tdisplay-esp32 -t upload     # firmware
```

## Controls

| Context | Button A | Button B |
| ------- | -------- | -------- |
| PIN entry | Cycle the current digit (0–9) | Confirm digit, move to the next |
| Dashboard | Cycle brightness (off → dim → normal → bright) | Force an immediate refresh |
| On boot | Hold **A+B** = factory reset (wipes all stored data) | |

## Notes

- Runs the original **Clarity** dashboard until it's migrated to the Mango tier S layout.
- During setup, the WiFi AP password is shown on the device screen.

---

**TTGO T Display ESP32** · env `tdisplay-esp32` · [All boards](Supported-Boards) · [Flashing](Flashing) · [Setup and daily use](Setup-and-Daily-Use) · [Troubleshooting](Troubleshooting)
