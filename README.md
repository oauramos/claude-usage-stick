# Claude Usage Stick

A tiny standalone device that shows your [Claude Code](https://docs.anthropic.com/en/docs/claude-code) rate-limit usage in real time. Polls the Anthropic API and displays your 5-hour and 7-day usage windows, reset countdowns, signal strength, and battery level.

Supports four boards:
- **M5StickC Plus** (ESP32-PICO, 240x135 LCD)
- **M5StickC Plus2** (ESP32-PICO-V3-02, 240x135 LCD)
- **LilyGo T-Display S3** (ESP32-S3, 320x170 LCD)
- **ESP32-C3-OLED** (ESP32-C3, 0.42" 72x40 OLED) — breadboard-friendly; bring your own buttons

<p align="center">
  <img src="docs/boot.jpg" width="260" alt="Boot screen">
  <img src="docs/pin.jpg" width="260" alt="PIN unlock">
  <img src="docs/dashboard.jpg" width="260" alt="Dashboard">
</p>

<p align="center">
  <img src="docs/esp32-c3-oled.jpg" width="400" alt="ESP32-C3-OLED running Claude Usage Stick">
</p>

## Features

- **Live usage bars** for the 5-hour and 7-day rate-limit windows
- **Reset countdowns** so you know when capacity frees up
- **PIN-protected** — your OAuth token is AES-256-GCM encrypted on-device; the PIN is never stored
- **Captive-portal setup** — connect your phone to the device's WiFi AP and configure everything in a browser
- **Battery & signal info** shown on the dashboard
- **Button controls** — cycle brightness (A), force refresh (B), factory reset (hold A+B on boot)

## Hardware

Use one of these supported boards:

| Board | MCU | Display | Battery | Buy |
| ----- | --- | ------- | ------- | --- |
| M5StickC Plus | ESP32-PICO | 1.14" 240x135 | 120 mAh | [m5stack.com](https://shop.m5stack.com/products/m5stickc-plus-esp32-pico-mini-iot-development-kit) |
| M5StickC Plus2 | ESP32-PICO-V3-02 | 1.14" 240x135 | 200 mAh | [m5stack.com](https://shop.m5stack.com/products/m5stickc-plus2-esp32-mini-iot-development-kit) |
| LilyGo T-Display S3 | ESP32-S3 | 1.9" 320x170 | 1300 mAh | [lilygo.cc](https://lilygo.cc/products/t-display-s3) |
| ESP32-C3-OLED | ESP32-C3 | 0.42" 72x40 OLED | external | [aliexpress.com](https://www.aliexpress.com/item/1005007929382296.html) |

Plus any USB-C cable for flashing and power.

### ESP32-C3-OLED wiring

The ESP32-C3-OLED module has no built-in buttons, so you wire two externally. The firmware expects both inputs to be **active-HIGH** (HIGH = pressed, LOW = idle) with internal pull-downs enabled.

| Signal | GPIO | Notes |
| ------ | ---- | ----- |
| Button A (cycle brightness / cycle digit) | GPIO 3 | active-HIGH |
| Button B (force refresh / confirm digit) | GPIO 7 | active-HIGH |
| I²C SDA | GPIO 5 | display (built-in) |
| I²C SCL | GPIO 6 | display (built-in) |
| Onboard LED | GPIO 8 | active-LOW (HIGH = off) |

> **Do not wire anything to GPIO 9 (BOOT/BOOT0)** — it is a strapping pin used for download mode.

#### Option A — tactile push-buttons

Wire each button between the GPIO pin and 3.3 V. When the button is open the internal pull-down holds the pin LOW; pressing it pulls it HIGH.

```
3.3 V ──┤button├── GPIO 3   (Button A)
3.3 V ──┤button├── GPIO 7   (Button B)
```

#### Option B — capacitive touch sensors

Any module that outputs a logic-HIGH signal when touched works as a drop-in replacement (e.g. TTP223-based pads). Wire the sensor's output to the GPIO pin and its power pins to 3.3 V and GND. The signal polarity and pull-down behaviour are identical to Option A.

<p align="center">
  <img src="docs/esp32-c3-oled-touch-buttons.jpg" width="500" alt="ESP32-C3-OLED wired with capacitive touch sensors on GPIO 3 and GPIO 7">
</p>

## How It Works

1. The device sends a minimal API request (`max_tokens: 1`) to the Anthropic Messages endpoint using your OAuth token
2. It reads the `anthropic-ratelimit-unified-5h-utilization` and `anthropic-ratelimit-unified-7d-utilization` response headers
3. The dashboard updates on a configurable interval (30s–5min)

The token never leaves the device. It is encrypted with AES-256-GCM using a key derived from your PIN (PBKDF via iterated SHA-256, 10 000 rounds).

## Setup

### Prerequisites

- [PlatformIO CLI](https://platformio.org/install/cli) installed
- A supported board connected via USB-C
- A Claude Code OAuth token (run `claude setup-token` in your terminal)

### Flash the firmware

```bash
# Clone the repo
git clone https://github.com/oauramos/claude-usage-stick.git
cd claude-usage-stick

# M5StickC Plus
pio run -e m5stick-cplus -t upload
pio run -e m5stick-cplus -t uploadfs

# M5StickC Plus2
pio run -e m5stick-cplus2 -t upload
pio run -e m5stick-cplus2 -t uploadfs

# LilyGo T-Display S3
pio run -e tdisplay-s3 -t upload
pio run -e tdisplay-s3 -t uploadfs

# ESP32-C3-OLED
pio run -e esp32c3-oled -t upload
pio run -e esp32c3-oled -t uploadfs
```

> **Apple Silicon note:** If `uploadfs` fails with "Bad CPU type", install Rosetta (`softwareupdate --install-rosetta`) or use the included Python fallback:
> ```bash
> python3 upload_data.py
> ```

### Configure the device

1. On first boot (or after factory reset), the device creates a WiFi access point named `ClaudeMonitor-XXXX`
2. Connect your phone or laptop to that network
   - **LCD boards** — the password is shown on the device screen
   - **ESP32-C3-OLED** — the AP is open (no password), since the OLED is too small to display one legibly
3. Open `http://192.168.4.1` in a browser
4. Fill in your WiFi credentials, OAuth token, and a 4–8 digit encryption PIN
5. Hit **Save & Reboot** — the device encrypts the token, stores it, and connects to your WiFi

### Daily use

On each boot, enter your PIN using the device buttons:

- **Button A** — cycle the current digit (0–9)
- **Button B** — confirm and move to the next digit

Once unlocked, the dashboard appears and auto-refreshes.

| Button | Dashboard action |
| ------ | ---------------- |
| A | Cycle screen brightness (off → dim → normal → bright); ESP32-C3-OLED: toggle on/off only |
| B | Force an immediate refresh |
| A+B held on boot | Factory reset (wipes all stored data) |

## Project Structure

```
src/
  main.cpp        — boot flow, WiFi, PIN entry, main loop
  hal.cpp/h       — hardware abstraction (display, buttons, battery, backlight)
  api.cpp/h       — HTTPS request to Anthropic, header parsing
  crypto.cpp/h    — AES-256-GCM encrypt/decrypt with PIN-derived key
  provision.cpp/h — captive portal WiFi AP + web server
  ui.cpp/h        — all LCD drawing (boot, PIN, dashboard, errors)
  config.h        — tunables (poll interval, timeouts, PIN attempts)
data/
  setup.html      — web UI served during provisioning
server/
  usage_proxy.py  — optional local caching proxy (reads token from macOS Keychain)
```

## Security

- The OAuth token is encrypted with AES-256-GCM before being written to NVS flash
- The encryption key is derived from your PIN + device MAC salt through 10 000 rounds of SHA-256
- The PIN is **never stored** — wrong PIN = failed decryption (GCM tag mismatch)
- After 10 failed PIN attempts, all credentials are wiped and the device resets to setup mode
- Lockout delay doubles after each failure (60s → 120s → 240s → ...)

## License

MIT
