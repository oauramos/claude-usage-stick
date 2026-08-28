# Web panel

*✨ Dust (v3) on the [LilyGo T-Display S3](LilyGo-T-Display-S3). Other boards will get it as they migrate to v3.*

Once the device is on your WiFi, it serves its own control panel to any browser on the same network. Everything you'd want to change after setup lives there — no re-flash, no factory reset, no serial console.

## Finding it

Three ways, all shown by the device itself:

- The dashboard **header alternates** between the device's name and its address, e.g. `http://192.168.2.136`.
- The **PIN screen** shows `unlock: http://<ip>` while the device is locked.
- The mDNS name **`http://claude-usage-stick.local`** works from macOS, iOS, Windows and most Linux — no router config needed. (Android's `.local` support is patchy; use the IP there.) If you set a device name during setup, the hostname is that name slugified — `Desk Stick` becomes `desk-stick.local`.

## Logging in

The panel's password is the **same 4-digit PIN** you enter on the device buttons. There is no separate account and nothing new to remember.

Two things worth knowing:

- **Logging in also unlocks the device.** If the stick is sitting at its PIN screen, a successful browser login decrypts the token and the screen jumps straight to the dashboard — typing the PIN in a browser beats cycling digits with two buttons.
- Wrong attempts slow down exponentially (HTTP 429) but **never wipe the device** — the 10-attempt wipe only counts PINs typed on the physical buttons. Sessions live in RAM: a reboot logs everyone out, and an idle session expires after 24 h. The full threat model is in [Security](Security#web-panel-t-display-s3-v3).

## What's on it

| Section | What it does |
| --- | --- |
| **Usage** | The live dashboard mirrored in the browser — bars, reset countdowns, model mascots — plus a **Refresh now** button |
| **Device** | Firmware version, hostname, IP, WiFi signal, uptime, free heap, and whether the screen is locked |
| **Display & refresh** | Brightness, refresh interval (30 s – 5 min), **timezone** (GMT±, feeds the clock screen and the chart's day markers), device name, flip 180° |
| **Screens** | The [display mode](The-UI#what-dust-adds) — static / carousel / clock — carousel dwell time, which screens rotate, and which model mascots render |
| **7-day history** | The same chart the device draws, rendered from the on-device history |
| **Anthropic news** | The headlines the news screen shows |
| **Claude token** | Replace the stored token — see below |
| **WiFi** | Scan networks and switch (the device reboots onto the new network) |
| **Danger zone** | Factory reset, armed only by typing `ERASE` |

## Rotating the token

The reason this panel exists. Tokens from `claude setup-token` expire after a year (or you may just want to revoke one), and until v3 the only way to swap tokens was a full factory reset.

1. Run `claude setup-token` in your terminal and copy the new token.
2. Open the panel → **Claude token**.
3. Paste the token, confirm with your PIN, hit **Replace token**.

The device re-encrypts it, swaps it live, and immediately test-drives it against the API so you get a *verified* / *failed* verdict on the spot. The token is **write-only**: no panel endpoint can ever read it back.

## Good to know

- The panel is plain **HTTP on your LAN** — same trust model as the setup portal. Don't port-forward it to the internet.
- While the device is mid-fetch (every refresh, and a news fetch every 6 h) the panel pauses for a second or two — it's a single-core gadget doing one thing at a time.
- Changing the device name applies to the header immediately, but the new `.local` hostname only takes effect on the next reboot.
