# Supported boards

Eight boards run the firmware today. Each has its own guide with pinouts, controls, and quirks — click the name.

| Board | MCU | Display | Firmware | PlatformIO env | Buy |
| ----- | --- | ------- | -------- | -------------- | --- |
| [M5StickC Plus](M5StickC-Plus) | ESP32-PICO | 1.14" 240×135 LCD | ✨ **Dust (v3.0.1)** · tier S | `m5stick-cplus` | [AliExpress](https://s.click.aliexpress.com/e/_c3w3hHWl) |
| [M5StickC Plus2](M5StickC-Plus2) | ESP32-PICO-V3-02 | 1.14" 240×135 LCD | Clarity (v1) | `m5stick-cplus2` | [AliExpress](https://s.click.aliexpress.com/e/_c3jkKlNj) |
| [LilyGo T-Display S3](LilyGo-T-Display-S3) | ESP32-S3 | 1.9" 320×170 LCD | ✨ **Dust (v3.0.1)** · tier L | `tdisplay-s3` | [AliExpress](https://s.click.aliexpress.com/e/_c4rvB1Mv) |
| [LilyGo T8 ESP32-S2](LilyGo-T8-ESP32-S2) | ESP32-S2 | 1.14" 135×240 LCD | Clarity (v1) | `t8-s2` | [AliExpress](https://s.click.aliexpress.com/e/_c2w1HnpJ) |
| [Elecrow CrowPanel Advance 3.5"](CrowPanel-Advance-35) | ESP32-S3 | 3.5" 480×320 IPS touch | Clarity (v1) | `crowpanel-adv-35` | [AliExpress](https://s.click.aliexpress.com/e/_c4lDErmN) |
| [LilyGo T-Display S3 AMOLED 1.91"](LilyGo-T-Display-S3-AMOLED) | ESP32-S3 | 1.91" 240×536 AMOLED | Clarity (v1) | `tdisplay-s3-amoled` | [AliExpress](https://s.click.aliexpress.com/e/_c3XNB9Hx) |
| [TTGO T-Display ESP32](TTGO-T-Display-ESP32) | ESP32 | 1.14" 135×240 LCD | Clarity (v1) | `tdisplay-esp32` | [AliExpress](https://s.click.aliexpress.com/e/_c32HlGQ1) |
| [ESP32-C3-OLED](ESP32-C3-OLED) | ESP32-C3 | 0.42" 72×40 OLED | Clarity (v1) | `esp32c3-oled` | [AliExpress](https://s.click.aliexpress.com/e/_c3JMxywv) |
| M5Stack StickS3 | — | — | 🚧 in progress | — | [AliExpress](https://s.click.aliexpress.com/e/_c3ZsWHBB) |

Plus any USB-C cable for flashing and power. Note that some cheap cables are **charge-only** — flashing needs a data cable.

## Which one should I buy?

- **Best overall:** [LilyGo T-Display S3](LilyGo-T-Display-S3) — the biggest small-format screen, two buttons, and the reference board for [tier L](The-UI#display-tiers) and the first to run ✨ Dust (web panel, screen carousel, 7-day chart).
- **Smallest complete package:** [M5StickC Plus](M5StickC-Plus) — case, battery, and buttons included, zero soldering, and it runs ✨ Dust tier S.
- **Cheapest:** [ESP32-C3-OLED](ESP32-C3-OLED) — but you wire your own buttons, and the 0.42" OLED shows a stripped-down layout.
- **Biggest screen:** [CrowPanel Advance 3.5"](CrowPanel-Advance-35) — touch instead of buttons; still on the Clarity dashboard.

## Board differences worth knowing

Not every board can do everything. These limits come from the hardware, not the firmware:

| Board | Battery readout | Factory reset on boot | Notes |
| ----- | --------------- | --------------------- | ----- |
| M5StickC Plus / Plus2 | ✅ | ✅ | Two buttons, internal battery |
| LilyGo T-Display S3 | ✅ | ✅ | Two buttons |
| TTGO T-Display | ✅ | ✅ | Two buttons |
| LilyGo T8 ESP32-S2 | ❌ | ❌ | One button (short tap / long press) |
| CrowPanel Advance 3.5" | ❌ | ❌ | Touch zones instead of buttons |
| T-Display S3 AMOLED | varies | ✅ | Button B needs a touch-equipped variant |
| ESP32-C3-OLED | ❌ | ✅ | You wire the buttons yourself |

Where factory reset on boot isn't available, re-flash the board to wipe its stored credentials.

## Adding a board

New boards are welcome — see [Project structure](Project-Structure#adding-a-board) for what's involved.
