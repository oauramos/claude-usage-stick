# Elecrow CrowPanel Advance 3.5" HMI

Part of [Claude Usage Stick](Home). The biggest screen in the lineup — a 3.5" 480×320 IPS HMI panel with capacitive touch and no physical buttons. Pin mapping comes from Elecrow's official LovyanGFX driver and is verified on hardware.

## Specs

| | |
| --- | --- |
| MCU | ESP32-S3-WROOM-1 N16R8 (dual-core, WiFi, 16 MB flash, 8 MB OPI PSRAM) |
| Display | 3.5" ILI9488 IPS, 480×320, SPI |
| Touch | GT911 capacitive, I²C (SDA 15, SCL 16) |
| Battery | External (percentage not shown) |
| Buttons | None — touch zones, see Controls |
| USB | CH340K UART bridge (not native USB CDC) |
| Firmware | Clarity (v1) — target display tier **XL** (not yet migrated) |
| PlatformIO env | `crowpanel-adv-35` |
| Buy | [aliexpress.com](https://s.click.aliexpress.com/e/_c4lDErmN) |

## Flash

Easiest: open the **[web flasher](https://oauramos.github.io/claude-usage-stick/)** in Chrome or Edge, pick this board, and plug it in over USB-C.

From source, with the [PlatformIO CLI](https://platformio.org/install/cli):

```bash
pio run -e crowpanel-adv-35 -t upload     # firmware
```

## Controls — touch zones

There are no physical user buttons, so the two-button UX maps to halves of the touch panel (matching the on-screen "tap left / tap right" legend):

| Touch | Acts as | PIN entry | Dashboard |
| ----- | ------- | --------- | --------- |
| Tap **LEFT** half | Button A | Cycle the current digit | Cycle brightness |
| Tap **RIGHT** half | Button B | Confirm digit | Force refresh |

## Notes

- **No on-boot factory reset** — the A+B combo needs two simultaneous inputs and only a single touch point is read. Re-flash to wipe NVS.
- **No battery readout** — battery percentage isn't shown on the dashboard.
- The dashboard renders into a PSRAM sprite and is pushed in one transfer, so refreshes don't flicker on this slow SPI panel; the PIN/setup screens draw straight to the panel so touch input stays responsive.
- During setup, the WiFi AP password is shown on the device screen.

---

**CrowPanel Advance 35** · env `crowpanel-adv-35` · [All boards](Supported-Boards) · [Flashing](Flashing) · [Setup and daily use](Setup-and-Daily-Use) · [Troubleshooting](Troubleshooting)
