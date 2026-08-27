# Claude Usage Stick

**Your [Claude Code](https://docs.anthropic.com/en/docs/claude-code) rate limits, glanceable on a tiny ESP32 stick.**

A standalone desk gadget that polls the Anthropic API and shows your rate-limit usage in real time — no computer, no app, no cloud. Flash it, connect it to WiFi from your phone, and it just sits there telling you how much runway you have left.

<img src="https://github.com/user-attachments/assets/c51c5a9e-5a3e-4c3e-97a8-d5a4c5a44263" width="640" alt="Claude Usage Stick — PIN unlock screen and usage dashboard on a LilyGo T-Display S3">

## Start here

| | |
| --- | --- |
| **New here?** | [Features](Features) — what the device actually shows |
| **Ready to build one?** | [Supported boards](Supported-Boards), then [Flashing](Flashing) |
| **Just flashed it?** | [Setup and daily use](Setup-and-Daily-Use) |
| **Something's wrong?** | [Troubleshooting](Troubleshooting) |

The fastest path to a working device: pick a board from the [supported list](Supported-Boards), open the **[web flasher](https://oauramos.github.io/claude-usage-stick/)** in Chrome or Edge, and plug the board in over USB-C. No toolchain, no drivers, no clone.

## All pages

**Using the device**
- [Features](Features) — usage bars, reset countdowns, model mascots, PIN lock
- [The UI](The-UI) — firmware versions (Clarity, Mango) and how the layout adapts per screen size
- [Setup and daily use](Setup-and-Daily-Use) — captive-portal setup, PIN entry, button controls
- [Troubleshooting](Troubleshooting) — flashing, WiFi, token, and display problems

**Hardware**
- [Supported boards](Supported-Boards) — the full lineup, with a guide for each
- Board guides: [M5StickC Plus](M5StickC-Plus) · [M5StickC Plus2](M5StickC-Plus2) · [LilyGo T-Display S3](LilyGo-T-Display-S3) · [LilyGo T8 ESP32-S2](LilyGo-T8-ESP32-S2) · [CrowPanel Advance 3.5"](CrowPanel-Advance-35) · [T-Display S3 AMOLED](LilyGo-T-Display-S3-AMOLED) · [TTGO T-Display](TTGO-T-Display-ESP32) · [ESP32-C3-OLED](ESP32-C3-OLED)

**Under the hood**
- [Flashing](Flashing) — the web flasher and building from source
- [How it works](How-It-Works) — the API request and the headers it reads
- [Security](Security) — how your OAuth token is encrypted on-device
- [Project structure](Project-Structure) — repo layout and how to add a board

## Project links

- [Repository](https://github.com/oauramos/claude-usage-stick) · [Web flasher](https://oauramos.github.io/claude-usage-stick/) · [Issues](https://github.com/oauramos/claude-usage-stick/issues)
- License: [MIT](https://github.com/oauramos/claude-usage-stick/blob/main/LICENSE)
