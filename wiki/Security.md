# Security

Your OAuth token grants access to your Claude account, and it lives on a small device on your desk. Here's exactly how it's protected.

## Token storage

- The token is encrypted with **AES-256-GCM** before it's written to NVS flash.
- The encryption key is derived from **your PIN plus the device's MAC address as salt**, through 10 000 rounds of SHA-256.
- The **PIN is never stored**, anywhere, in any form. A wrong PIN produces a wrong key, and decryption fails on the GCM authentication tag — the device can tell the PIN was wrong without ever having kept a copy of the right one.

The practical consequence: if you forget the PIN, the token is unrecoverable. Factory reset and set the device up again.

## Brute-force protection

- After **10 consecutive failed PIN attempts**, the device wipes all stored credentials and returns to setup mode.
- The lockout delay **doubles after each failure**: 60 s, then 120 s, then 240 s, and so on. Ten guesses take a long time.

## What this does and doesn't protect against

**Protected:** someone who picks up your device and tries to read the token off it, or guesses PINs at the screen. Without the PIN, the flash contents are ciphertext.

**Not protected:** someone with physical access, a soldering iron, and the willingness to attack the flash chip directly while you're using the device. This is a desk gadget, not a hardware security module. Treat the token accordingly — it's a Claude Code token, and you can revoke and reissue it at any time with `claude setup-token`.

## Network

The device talks to `api.anthropic.com` over HTTPS, to `status.claude.com` for model health, and (v3, for the news screen) to `raw.githubusercontent.com` for the Anthropic news feed. There's no backend, no telemetry, and no update channel — see [How it works](How-It-Works).

On the T-Display S3 (v3 firmware) the device also **listens** on your LAN: plain HTTP on port 80, serving the [web panel](Web-Panel), plus mDNS so `claude-usage-stick.local` resolves. Nothing is exposed beyond your network unless you port-forward it yourself — don't.

## Web panel (T-Display S3, v3)

The panel's login is the **same 4-digit PIN** you enter on the buttons. There is still no stored password anywhere: a login attempt simply tries to decrypt the token blob with the submitted PIN, and the AES-GCM tag says yes or no.

What that means in practice:

- **The PIN crosses your LAN in plain HTTP at login.** The ESP32 can't serve browser-trusted TLS, so this is the same trust model as the setup portal: fine on a home network, not for a hostile one. Treat the panel like the device itself — anyone on your LAN who knows the PIN owns it.
- **Web login failures never wipe the device.** Wrong attempts get an exponential slow-down (HTTP 429) and each guess pays the full 10,000-round key derivation, but only wrong PINs typed **on the physical buttons** count toward the 10-attempt credential wipe. A neighbor on your WiFi can't erase your device by hammering the login.
- **Sessions live in RAM only** — HttpOnly, SameSite=Strict cookies that expire after 24 h idle and die on every reboot.
- **The token is write-only.** No panel endpoint ever returns the token, the WiFi password, or the PIN; the token can only be *replaced*, and replacing it re-requires the PIN.
- A successful web login while the device sits at the PIN screen also unlocks the screen — it just decrypted the token, which is the same proof the buttons provide.

During setup the device runs an open-ish access point (`ClaudeMonitor-XXXX`, password shown on its screen) that serves a plain HTTP form on `192.168.4.1`. That's a brief window on a local AP with a password, and it closes as soon as you hit Save & Reboot — but it does mean you shouldn't do first-time setup somewhere hostile, like a crowded conference.

## Rotating the token

On the T-Display S3 (v3): run `claude setup-token`, open the [web panel](Web-Panel), and paste the new token together with your PIN — no factory reset, WiFi and settings stay put.

On every other board: factory reset the device (hold **A+B** on boot, or re-flash on boards that can't), then run `claude setup-token` again and redo [setup](Setup-and-Daily-Use).
