# M5StickC Plus

Part of [Claude Usage Stick](Home). The original board this project was built for — a finger-sized ESP32 stick with a built-in display, battery, and two buttons. Zero soldering required.

## Specs

| | |
| --- | --- |
| MCU | ESP32-PICO-D4 (dual-core, WiFi) |
| Display | 1.14" ST7789 LCD, 240×135 |
| Battery | 120 mAh internal |
| Buttons | Button A (front, GPIO 37) · Button B (side, GPIO 39) |
| Firmware | ✨ **Dust (v3.0.2)** — display tier **S** (reference board) |
| PlatformIO env | `m5stick-cplus` |
| Buy | [aliexpress.com](https://s.click.aliexpress.com/e/_c3w3hHWl) |

## Flash

Easiest: open the **[web flasher](https://oauramos.github.io/claude-usage-stick/)** in Chrome or Edge, pick this board, and plug it in over USB-C.

From source, with the [PlatformIO CLI](https://platformio.org/install/cli):

```bash
pio run -e m5stick-cplus -t upload     # firmware
```

## Controls

| Context | Gesture | Action |
| ------- | ------- | ------ |
| PIN entry | A tap / B tap | Cycle the current digit / confirm it — or unlock from a [browser](Web-Panel) instead |
| Screens | A tap (front button) | Next screen (dashboard → chart → news → clock) |
| Screens | A held ≥ 0.6 s | Flip screen 180° (saved) |
| Screens | B tap (side) | Cycle brightness (saved) |
| Screens | A+B together | Force refresh |
| On boot | Hold **A+B** | Factory reset (wipes all stored data) |

Refresh happens automatically on the poll interval. Everything else — display mode, timezone, refresh interval, token rotation, WiFi — lives in the **[web panel](Web-Panel)**.

## Notes

- As the **tier S** reference board, the dashboard's MODELS section shows one overall-health Clawd mascot plus a 2×2 `NAME UP/DOWN` text grid — see [Display tiers](The-UI#display-tiers). The [Dust screens](The-UI#what-dust-adds) (7-day chart, news, clock) are laid out for the 240×135 panel.
- Dust on this board uses the `min_spiffs` partition table: the app slot grows to 1.9 MB inside the 4 MB flash, and the 192 KB data partition stores the 7-day history. Updating from v2 keeps your settings.
- No PSRAM here — the flicker-free render buffer lives in internal RAM (~65 KB), which the ESP32 has room for.
- During setup, the WiFi AP password is shown on the device screen. An unreachable WiFi at boot opens a **recovery AP** (token, PIN and settings kept).

---

**M5StickC Plus** · env `m5stick-cplus` · [All boards](Supported-Boards) · [Flashing](Flashing) · [Setup and daily use](Setup-and-Daily-Use) · [Troubleshooting](Troubleshooting)
