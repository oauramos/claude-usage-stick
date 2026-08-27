# LilyGo T8 ESP32-S2

Part of [Claude Usage Stick](Home). A bare-bones ESP32-S2 board with the same 1.14" ST7789 panel as the TTGO T-Display, wired over the broken-out FSPI pins. Verified on hardware — display, WiFi provisioning, the encrypted dashboard, and button input all work.

## Specs

| | |
| --- | --- |
| MCU | ESP32-S2 (single-core, WiFi) |
| Display | 1.14" ST7789 LCD, 135×240 (FSPI: MOSI 35, SCLK 36, CS 34, DC 37, RST 38, BL 33) |
| Battery | External via JST connector (no battery-sense ADC — percentage not shown) |
| Buttons | Onboard BOOT button only (GPIO 0) — see Controls |
| USB | CH340C UART bridge (not native USB CDC) |
| Firmware | Clarity (v1) — target display tier **S** (not yet migrated) |
| PlatformIO env | `t8-s2` |
| Buy | [aliexpress.com](https://s.click.aliexpress.com/e/_c2w1HnpJ) |

## Flash

Easiest: open the **[web flasher](https://oauramos.github.io/claude-usage-stick/)** in Chrome or Edge, pick this board, and plug it in over USB-C.

From source, with the [PlatformIO CLI](https://platformio.org/install/cli):

```bash
pio run -e t8-s2 -t upload     # firmware
pio run -e t8-s2 -t uploadfs   # web setup UI (SPIFFS)
```

## Controls — one button, two roles

The board exposes only the onboard **BOOT** button (GPIO 0), so the two-button UX is folded onto a single button by press duration:

| Press | Acts as | PIN entry | Dashboard |
| ----- | ------- | --------- | --------- |
| Short tap | Button A | Cycle the current digit | Cycle brightness |
| Long press | Button B | Confirm digit | Force refresh |

## Notes

- **No on-boot factory reset** — GPIO 0 is a strapping pin (holding it during reset enters download mode), so the "hold A+B on boot" combo is unavailable. Re-flash to wipe NVS.
- **No battery readout** — there's no confirmed battery-sense ADC, so battery percentage isn't shown on the dashboard.
- During setup, the WiFi AP password is shown on the device screen.

---

**LilyGo T8 ESP32 S2** · env `t8-s2` · [All boards](Supported-Boards) · [Flashing](Flashing) · [Setup and daily use](Setup-and-Daily-Use) · [Troubleshooting](Troubleshooting)
