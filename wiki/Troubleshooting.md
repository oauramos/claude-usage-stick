# Troubleshooting

## Flashing

### The web flasher doesn't show a Connect button

Web Serial only exists in **Chrome, Edge, and Opera on desktop**. Firefox and Safari don't implement it, and neither does any browser on iOS or Android. The page will say so rather than failing silently.

### The board isn't in the serial-port list

- Use a **data** USB-C cable. Charge-only cables are common and carry no data lines.
- Hold the **BOOT** button while plugging the board in to force download mode.
- On macOS and Windows, some boards need a USB-UART driver: **CH340/CH341** for the T8-S2, CrowPanel, and TTGO T-Display; **CP210x** for various others. Boards with native USB (ESP32-S3, C3) usually need nothing.

### My board says "build pending"

Its firmware image hasn't been published by CI. Build it from source instead — see [Flashing](Flashing#from-source).

### `uploadfs` fails with "Bad CPU type" on a Mac

The bundled mkspiffs binary is x86-only. Install Rosetta (`softwareupdate --install-rosetta`) or run the repo's `python3 upload_data.py` fallback.

## WiFi and setup

### I can't find the `ClaudeMonitor-XXXX` network

Give the device 10–15 seconds after boot. If it never appears, it's probably already configured — factory reset it (hold **A+B** during boot) to return it to setup mode.

### The setup page won't load

Browse to **`http://192.168.4.1`** explicitly, with `http://`, not https. Turn off mobile data or any VPN first — phones often route the request to the cellular network instead of the device's AP.

### It connects to WiFi, then drops

ESP32 radios are **2.4 GHz only**. If your router advertises one name for both bands, the device may be handed a 5 GHz association it can't keep. Give the 2.4 GHz band its own SSID, or temporarily disable band steering while setting up.

## Usage display

### The device shows `no_usage_h_200`

The request succeeded (HTTP 200), but the response carried **no unified usage headers**. This means the token is valid and the account is fine — but the plan behind it doesn't publish 5h/7d usage.

**Enterprise and API-billed accounts don't emit these headers.** The device needs a token from a **Claude Pro or Max** subscription. Generate one with `claude setup-token` while signed into the subscription account, then redo [setup](Setup-and-Daily-Use).

### Usage stays at 0%

You genuinely haven't used any of the window yet, which is the happy case. If you have been working, check that the token belongs to the same account you're using Claude Code with.

### The dashboard freezes or stops refreshing

Check the WiFi icon in the header. If the signal dropped, the device retries on its own. If it stays stuck, power-cycle it — credentials survive a reboot, but you'll re-enter your PIN.

## PIN

### I forgot my PIN

There's no recovery — that's the design, not an oversight. The PIN is never stored, so nothing on the device can verify or reveal it. Factory reset (hold **A+B** on boot, or re-flash on boards without that combo) and set it up again with a new token. See [Security](Security).

### The device wiped itself

Ten consecutive wrong PIN attempts trigger a full credential wipe. Set it up again from scratch.

## Still stuck?

Open an [issue](https://github.com/oauramos/claude-usage-stick/issues) with your board, the firmware version, and what the screen shows.
