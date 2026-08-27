# Features

What the device shows once it's on your desk. For how the layout changes per screen size, see [The UI](The-UI).

## Live usage bars

Two bars, one per rate-limit window: the **5-hour** and the **7-day**. They refresh on a configurable interval (30 s to 5 min) straight from the Anthropic API — see [How it works](How-It-Works).

## Reset countdowns

Each window shows how long until capacity frees up, so you know whether to keep going or take a break. On [tier L](The-UI#display-tiers) the countdowns get their own large-type row; on tier S they sit inline on each bar.

## Model status mascots

*Mango (v2) only.* Haiku, Sonnet, Opus and Fable health, pulled from [status.claude.com](https://status.claude.com) and drawn as Clawd mascots. A healthy model blinks; a downed one turns gray with X eyes. Small screens show the same information as a `NAME UP/DOWN` text grid instead of mascots.

## PIN-protected token

Your OAuth token is AES-256-GCM encrypted on the device's flash, with the key derived from a 4-digit PIN you choose. The PIN itself is never stored. Full details in [Security](Security).

## Captive-portal setup

No hardcoded credentials and no serial console. On first boot the device opens its own WiFi access point; you join it from your phone and fill in a form. Walkthrough in [Setup and daily use](Setup-and-Daily-Use).

## Battery and signal

Battery level and WiFi signal strength appear as icons in the dashboard header — on boards that have a battery-sense ADC. A few boards can't read battery voltage; their guides say so.

## Button controls

Brightness, screen flip, force refresh, and factory reset, all from the board's buttons. The exact mapping differs per board (some have two buttons, some one, some only a touch screen) — see [Setup and daily use](Setup-and-Daily-Use#controls) or your [board's guide](Supported-Boards).
