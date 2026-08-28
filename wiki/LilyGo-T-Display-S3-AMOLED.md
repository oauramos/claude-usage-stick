# LilyGo T-Display S3 AMOLED (1.91")

Part of [Claude Usage Stick](Home). The AMOLED variant of the T-Display S3 — a long, narrow 240×536 RM67162 panel. One build covers every 1.91" revision: the panel variant (H712 / H713 / H705 / H681 / H717) is auto-detected at runtime by the LilyGo_AMOLED library, touch and non-touch, V1.0/V2.0/Black Shell alike.

## Specs

| | |
| --- | --- |
| MCU | ESP32-S3 (dual-core, WiFi, PSRAM, 16 MB flash) |
| Display | 1.91" RM67162 AMOLED, 240×536 |
| Touch | On H705 / H681 / H717 variants only |
| Battery | Varies by variant (JST connector) |
| Buttons | Single physical button (GPIO 0) — see Controls |
| Firmware | Clarity (v1) — target display tier **XL** (not yet migrated) |
| PlatformIO env | `tdisplay-s3-amoled` |
| Buy | [aliexpress.com](https://s.click.aliexpress.com/e/_c3XNB9Hx) |

## Flash

Easiest: open the **[web flasher](https://oauramos.github.io/claude-usage-stick/)** in Chrome or Edge, pick this board, and plug it in over USB-C.

From source, with the [PlatformIO CLI](https://platformio.org/install/cli):

```bash
pio run -e tdisplay-s3-amoled -t upload     # firmware
```

> This env is for the **1.91" AMOLED variant**. For the regular LCD version, use [`tdisplay-s3`](LilyGo-T-Display-S3).

## Controls

| Input | Acts as | PIN entry | Dashboard |
| ----- | ------- | --------- | --------- |
| Physical button (GPIO 0) | Button A | Cycle the current digit | Cycle brightness |
| Tap the screen anywhere | Button B | Confirm digit | Force refresh |

## Notes

- **Button B requires touch** — on non-touch variants (H712 / H713) there is no Button B input, so the touch-equipped revisions are the better pick.
- During setup, the WiFi AP password is shown on the device screen.

---

**LilyGo T Display S3 AMOLED** · env `tdisplay-s3-amoled` · [All boards](Supported-Boards) · [Flashing](Flashing) · [Setup and daily use](Setup-and-Daily-Use) · [Troubleshooting](Troubleshooting)
