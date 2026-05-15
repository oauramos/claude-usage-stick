# Claude Usage Stick

A tiny standalone device that shows your [Claude Code](https://docs.anthropic.com/en/docs/claude-code) rate-limit usage in real time. Polls the Anthropic API and displays your 5-hour and 7-day usage windows, reset countdowns, signal strength, and battery level.

Supports four boards:
- **M5StickC Plus** (ESP32-PICO, 240x135 LCD)
- **M5StickC Plus2** (ESP32-PICO-V3-02, 240x135 LCD)
- **LilyGo T-Display S3** (ESP32-S3, 320x170 LCD)
- **LilyGo T-Display S3 AMOLED** 1.91" (ESP32-S3, 240x536 RM67162 AMOLED — H712/H713/H705/H681/H717)

<p align="center">
  <img src="docs/boot.jpg" width="260" alt="Boot screen">
  <img src="docs/pin.jpg" width="260" alt="PIN unlock">
  <img src="docs/dashboard.jpg" width="260" alt="Dashboard">
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
| LilyGo T-Display S3 | ESP32-S3 | 1.9" 320x170 LCD | 1300 mAh | [lilygo.cc](https://lilygo.cc/products/t-display-s3) |
| LilyGo T-Display S3 AMOLED (1.91") | ESP32-S3 | 1.91" 240x536 AMOLED | varies | [lilygo.cc](https://lilygo.cc/products/t-display-s3-amoled) |

Plus any USB-C cable for flashing and power.

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

# — or — LilyGo T-Display S3 (regular LCD variant)
pio run -e tdisplay-s3 -t upload
pio run -e tdisplay-s3 -t uploadfs

# — or — LilyGo T-Display S3 AMOLED 1.91" (H712/H713/H705/H681/H717)
pio run -e tdisplay-s3-amoled -t upload
pio run -e tdisplay-s3-amoled -t uploadfs
```

> **AMOLED note:** the panel variant is auto-detected at runtime by the LilyGo_AMOLED library, so a single `tdisplay-s3-amoled` build covers all 1.91" AMOLED revisions (touch and non-touch, V1.0/V2.0/Black Shell). On touch-equipped variants (H705/H681/H717), tapping the screen anywhere acts as Button B.

> **Apple Silicon note:** If `uploadfs` fails with "Bad CPU type", install Rosetta (`softwareupdate --install-rosetta`) or use the included Python fallback:
> ```bash
> python3 upload_data.py
> ```

### Configure the device

1. On first boot (or after factory reset), the device creates a WiFi access point named `ClaudeMonitor-XXXX`
2. Connect your phone or laptop to that network using the password shown on the device screen
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
| A | Cycle screen brightness (off → dim → normal → bright) |
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
