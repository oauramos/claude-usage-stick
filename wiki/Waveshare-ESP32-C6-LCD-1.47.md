# Waveshare ESP32-C6-LCD-1.47

Part of [Claude Usage Stick](Home). A small RISC-V board with a 1.47" LCD, Wi-Fi 6 and one usable button. It is the first ESP32-C6 board in the lineup, and the first that builds on Arduino core 3.x.

## Specs

| | |
| --- | --- |
| MCU | ESP32-C6 (single-core RISC-V, 160 MHz, Wi-Fi 6, BLE 5, 802.15.4), no PSRAM |
| Flash | 8 MB on the unit tested (ESP32-C6FH8), though the listing says 4 MB |
| Display | 1.47" ST7789 LCD, 172×320 over SPI, used in landscape (320×172) |
| Battery | None (USB-C power) |
| Buttons | BOOT (GPIO 9) only. The other button is RESET. |
| Firmware | ✨ **Dust (v3.1.0)**, display tier **L** (same layout as the T-Display S3) |
| PlatformIO env | `waveshare-c6-lcd` |
| Buy | [waveshare.com](https://www.waveshare.com/esp32-c6-lcd-1.47.htm) |

## Flash

Easiest: open the **[web flasher](https://oauramos.github.io/claude-usage-stick/)** in Chrome, Edge or Firefox, pick this board, and plug it in over USB-C.

From source, with the [PlatformIO CLI](https://platformio.org/install/cli):

```bash
pio run -e waveshare-c6-lcd -t upload     # firmware
```

The first build downloads the [pioarduino](https://github.com/pioarduino/platform-espressif32) platform. PlatformIO's own `espressif32` platform has no Arduino support for the C6.

## Controls

The board has one usable button, so a tap and a hold act as the two buttons of the other boards.

| Context | Gesture | Action |
| ------- | ------- | ------ |
| PIN entry | BOOT tap / BOOT hold ≥ 0.7 s | Step the current digit / confirm it. You can also unlock from a [browser](Web-Panel). |
| Screens | BOOT tap | Next screen (dashboard → chart → news → clock) |
| Screens | BOOT hold ≥ 0.7 s | Cycle brightness (saved) |

Flip screen, force refresh and factory reset need two buttons on the other boards. On this board, use the **[web panel](Web-Panel)** for them.

## Notes

- The display uses **LovyanGFX**, not TFT_eSPI. TFT_eSPI 2.5.43 has no ESP32-C6 driver. Its C3 driver compiles for the C6 if you patch it, but it does not reach the panel. LovyanGFX supports the C6 and has the same API, so `ui.cpp` is unchanged. The panel config (34-column offset, IPS inversion, PWM backlight) is in `src/hal.h`.
- The panel is 172 rows tall in landscape. The tier L layout is drawn for 170 rows, so the bottom 2 rows stay blank.
- There is no PSRAM. The Dust screen sprite (320×172, ~110 KB) uses internal RAM.
- Opening the USB serial port (a serial monitor, for example) resets the board. This is how the C6's built-in USB-Serial/JTAG works, not a crash.
- During setup, the Wi-Fi AP password shows on the screen. Do not hold BOOT while you press RESET or plug in the board. That starts the ROM download mode, not the firmware.

---

**Waveshare ESP32-C6-LCD-1.47** · env `waveshare-c6-lcd` · [All boards](Supported-Boards) · [Flashing](Flashing) · [Setup and daily use](Setup-and-Daily-Use) · [Troubleshooting](Troubleshooting)
