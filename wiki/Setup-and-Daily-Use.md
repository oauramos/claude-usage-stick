# Setup and daily use

Everything after [flashing](Flashing): getting the device onto your WiFi, then living with it.

## What you need

- A Claude Code OAuth token. Run this in your terminal and copy the result:
  ```bash
  claude setup-token
  ```
- Your WiFi network name and password (2.4 GHz — ESP32 boards don't do 5 GHz).
- A 4-digit PIN you'll remember. It encrypts the token and is **never stored**, so there is no recovery if you forget it.

## First-time setup

On first boot — or after a factory reset — the device starts a captive portal.

1. The device screen shows a WiFi network named `ClaudeMonitor-XXXX` along with its password. Join that network from your phone or laptop.
2. Open **`http://192.168.4.1`** in a browser. (Most phones pop the page up automatically.)
3. Fill in the form: WiFi credentials, your OAuth token, and your 4-digit PIN.
4. Hit **Save & Reboot**.

The device encrypts the token with your PIN, stores it, and connects to your WiFi. See [Security](Security) for what that encryption actually does.

> On the [ESP32-C3-OLED](ESP32-C3-OLED) the screen is too small for the full random password, so it shows a simpler 8-digit one instead.

## Daily use

On every boot you enter your PIN with the board's buttons:

- **Button A** cycles the current digit (0–9)
- **Button B** confirms it and moves to the next

Once unlocked, the dashboard appears and refreshes on its own.

On ✨ **Dust** (v3) boards there's a second way: logging into the **[web panel](Web-Panel)** from any browser on your network unlocks the screen too — no button-cycling. Reach it at `claude-usage-stick.local` (or your device-name slug); by design the locked screen doesn't display the address — it appears in the dashboard header only after unlock. The panel is also where settings, token rotation and WiFi changes live after setup.

## Controls

| Button | Clarity (v1) | 🥭 Mango (v2) | ✨ Dust (v3) |
| ------ | ------------ | ------------- | ------------ |
| A | Cycle brightness | Flip screen 180° | Next screen |
| A held ≥ 0.6 s | — | — | Flip screen 180° (saved) |
| B | Force refresh | Cycle brightness | Cycle brightness (saved) |
| A+B | — | Force refresh | Force refresh |
| A+B held on boot | Factory reset | Factory reset | Factory reset |

Boards without two buttons map this differently — one button by press duration, or touch zones on the touch panels. Check your [board's guide](Supported-Boards):

- [LilyGo T8 ESP32-S2](LilyGo-T8-ESP32-S2) — short tap = A, long press = B
- [CrowPanel Advance 3.5"](CrowPanel-Advance-35) — tap left half = A, tap right half = B
- [T-Display S3 AMOLED](LilyGo-T-Display-S3-AMOLED) — physical button = A, tap the screen = B
- [ESP32-C3-OLED](ESP32-C3-OLED) — buttons you wire yourself on GPIO 3 and GPIO 7

## Factory reset

Holding **A+B** while the device boots wipes every stored credential and returns it to setup mode.

On ✨ Dust boards you rarely need it: WiFi changes and [token rotation](Web-Panel#rotating-the-token) both live in the web panel, an unreachable network drops the device into a recovery portal by itself, and the panel's *Danger zone* can trigger the same full wipe from the browser.

Three boards can't do it in hardware — the T8-S2 and CrowPanel have no way to signal two simultaneous inputs, and GPIO 0 is a strapping pin. Re-flash those boards to wipe their storage.

Note that the device also wipes itself after **10 consecutive wrong PIN attempts**, with the lockout delay doubling each time (60 s → 120 s → 240 s …).
