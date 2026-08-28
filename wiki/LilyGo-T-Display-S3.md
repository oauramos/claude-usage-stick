# LilyGo T-Display S3

Part of [Claude Usage Stick](Home). The biggest small-format screen in the lineup — a 1.9" LCD driven over a fast 8-bit parallel bus, with two front buttons and a battery connector.

## Specs

| | |
| --- | --- |
| MCU | ESP32-S3 (dual-core, WiFi, 8 MB PSRAM) |
| Display | 1.9" ST7789 LCD, 320×170, 8-bit parallel |
| Battery | External via JST connector (not included) |
| Buttons | Button A (BOOT, GPIO 0) · Button B (KEY, GPIO 14) |
| Firmware | ✨ **Dust (v3.0.3)** — display tier **L** (reference board) |
| PlatformIO env | `tdisplay-s3` |
| Buy | [aliexpress.com](https://s.click.aliexpress.com/e/_c4rvB1Mv) |

## Flash

Easiest: open the **[web flasher](https://oauramos.github.io/claude-usage-stick/)** in Chrome or Edge, pick this board, and plug it in over USB-C.

From source, with the [PlatformIO CLI](https://platformio.org/install/cli):

```bash
pio run -e tdisplay-s3 -t upload     # firmware
```

> This env is for the **regular LCD variant**. For the 1.91" AMOLED version, use [`tdisplay-s3-amoled`](LilyGo-T-Display-S3-AMOLED).

## Controls

| Context | Gesture | Action |
| ------- | ------- | ------ |
| PIN entry | A tap / B tap | Cycle the current digit / confirm it — or unlock from a [browser](Web-Panel) instead |
| Screens | A tap | Next screen (dashboard → chart → news → clock) |
| Screens | A held ≥ 0.6 s | Flip screen 180° (saved) |
| Screens | B tap | Cycle brightness (saved) |
| Screens | A+B together | Force refresh |
| On boot | Hold **A+B** | Factory reset (wipes all stored data) |

Refresh happens automatically on the poll interval. Everything else — display mode, timezone, refresh interval, token rotation, WiFi — lives in the **[web panel](Web-Panel)**.

## Notes

- As the **tier L** reference board, the dashboard shows the 5H/7D reset countdowns in large type on their own row below the bars, and the MODELS section shows a row of labelled Clawd mascots (Haiku / Sonnet / Opus / Fable), each blinking while healthy — see [Display tiers](The-UI#display-tiers).
- Dust adds three more screens — [7-day chart, Anthropic news, and a desk clock](The-UI#what-dust-adds) — plus the [web panel](Web-Panel) and mDNS (`claude-usage-stick.local`).
- v3 uses the board's full 16 MB flash (the app partition grew 5×) and enables the 8 MB PSRAM. Updating from v2 via the web flasher keeps your settings; a clean erase re-runs setup.
- During setup, the WiFi AP password is shown on the device screen. If the device can't reach your WiFi at boot (moved house, new router), it opens a **recovery AP** after ~1 min where you fix the WiFi without losing token, PIN or settings.

---

**LilyGo T Display S3** · env `tdisplay-s3` · [All boards](Supported-Boards) · [Flashing](Flashing) · [Setup and daily use](Setup-and-Daily-Use) · [Troubleshooting](Troubleshooting)
