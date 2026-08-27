# Flashing

Two ways to get the firmware onto a board. The web flasher needs nothing installed; building from source is for development or for boards whose CI build isn't published yet.

## From your browser (recommended)

Open the **[web flasher](https://oauramos.github.io/claude-usage-stick/)**, pick your board, plug it in over USB-C, and click Flash.

Requirements:

- **Chrome, Edge, or Opera on desktop.** The flasher uses the Web Serial API, which Firefox, Safari, and every browser on iOS/Android do not implement.
- **A USB-C data cable.** Charge-only cables power the board but carry no data.
- If the board isn't detected, hold **BOOT** while plugging it in to force download mode.

Firmware images are built from `main` by CI and published alongside the page, so the flasher always serves the current build. A board whose build hasn't landed is grayed out and labelled *build pending*.

## From source

You need the [PlatformIO CLI](https://platformio.org/install/cli). Pick your board's env from [Supported boards](Supported-Boards):

```bash
git clone https://github.com/oauramos/claude-usage-stick.git
cd claude-usage-stick

pio run -e <env> -t upload      # firmware
pio run -e <env> -t uploadfs    # web setup UI (SPIFFS)
```

### Apple Silicon note

If `uploadfs` fails with *"Bad CPU type"*, the bundled mkspiffs binary is x86-only. Either install Rosetta:

```bash
softwareupdate --install-rosetta
```

or use the Python fallback included in the repo:

```bash
python3 upload_data.py
```

### Known build issues

Two envs currently fail to build in some environments for reasons unrelated to the firmware itself:

- **`m5stick-cplus2`** — a dependency pin conflict
- **`tdisplay-s3-amoled`** — an `intelhex` failure in the LilyGo AMOLED toolchain

These are environmental, not regressions. When they fail in CI, the affected board simply shows as *build pending* on the web flasher; every other board still deploys.

## After flashing

The device reboots into setup mode and opens its own WiFi access point — continue with [Setup and daily use](Setup-and-Daily-Use).
