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

The device talks to `api.anthropic.com` over HTTPS and to `status.claude.com` for model health. Nothing else. There's no backend, no telemetry, and no update channel — see [How it works](How-It-Works).

During setup the device runs an open-ish access point (`ClaudeMonitor-XXXX`, password shown on its screen) that serves a plain HTTP form on `192.168.4.1`. That's a brief window on a local AP with a password, and it closes as soon as you hit Save & Reboot — but it does mean you shouldn't do first-time setup somewhere hostile, like a crowded conference.

## Rotating the token

Factory reset the device (hold **A+B** on boot, or re-flash on boards that can't), then run `claude setup-token` again and redo [setup](Setup-and-Daily-Use).
