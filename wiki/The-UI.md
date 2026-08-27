# The UI

Firmware releases carry names. Which one your board runs is listed in [Supported boards](Supported-Boards).

## Versions

| Version | Name | Highlights |
| ------- | ---- | ---------- |
| v1 | **Clarity** | The original dashboard — usage bars, reset countdowns, PIN unlock, captive-portal setup |
| v2 | 🥭 **Mango** | Everything in Clarity, plus model-status mascots, header icons, inline countdowns, and screen flip |

Boards still on **Clarity** keep the original minimal dashboard until they're migrated to their tier.

## Display tiers

The Mango dashboard keeps the same header and usage bars on every board, and adapts only its bottom **MODELS** section to the screen size. Each tier has a reference board; the layout scales to fit.

| Tier | Class | Resolution | Reference board | MODELS section | Status |
| ---- | ----- | ---------- | --------------- | -------------- | ------ |
| **XS** | tiny OLED | ≤ 128×64 | [ESP32-C3-OLED](ESP32-C3-OLED) | — | ⏳ Pending |
| **S** | small LCD | ~240×135 | [M5StickC Plus](M5StickC-Plus) | One overall-health Clawd + a 2×2 `NAME UP/DOWN` text grid | ✅ |
| **L** | large LCD | ~320×170 | [LilyGo T-Display S3](LilyGo-T-Display-S3) | A row of four labelled Clawds (one per model), each blinking when healthy | ✅ |
| **XL** | big / touch | ≥ 480×320 | [CrowPanel 3.5"](CrowPanel-Advance-35), [S3 AMOLED](LilyGo-T-Display-S3-AMOLED) | — | ⏳ Pending |

## What Mango adds

- **Model status mascots** — Haiku / Sonnet / Opus / Fable health from the Claude status page; a downed model turns gray with X eyes, healthy ones blink
- **Header icons** — battery level and WiFi signal strength as icons in the header bar
- **Reset countdowns by tier** — tier S shows each reset time inline on its bar row; tier L (v2.1.1) gives the countdowns their own large-type row below the bars
- **Dashboard-styled PIN screen** — the unlock screen matches the dashboard look
- **Screen flip and brightness** — Button A flips the screen 180°, Button B cycles brightness; refresh happens automatically, or press **A+B** together to force one

## Screens

| Boot | PIN unlock | Dashboard |
| --- | --- | --- |
| <img src="https://raw.githubusercontent.com/oauramos/claude-usage-stick/main/assets/boot.jpg" width="240" alt="Boot screen"> | <img src="https://raw.githubusercontent.com/oauramos/claude-usage-stick/main/assets/pin.jpg" width="240" alt="PIN unlock screen"> | <img src="https://raw.githubusercontent.com/oauramos/claude-usage-stick/main/assets/dashboard.jpg" width="240" alt="Usage dashboard"> |
