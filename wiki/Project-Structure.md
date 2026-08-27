# Project structure

```
assets/           — images (hero, gallery, wiring photos)
wiki/             — the source of these wiki pages; CI publishes it to the wiki
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
web/              — the browser-based flasher published to GitHub Pages
platformio.ini    — one build env per board
```

## Editing these docs

The wiki is **generated from the repo**. Pages live in `wiki/` on `main`, and a GitHub Action pushes them to the wiki whenever they change — so documentation gets reviewed in pull requests like any other change.

Edit the file in `wiki/`, open a PR, and the wiki updates on merge. Editing a page directly in the wiki UI works too, but the next sync will overwrite it.

## Editing the web flasher

`web/index.html` and `web/manifests/*.json` are generated. Edit `web/src/template.html` and run:

```bash
python3 web/src/build.py
```

The board list, firmware versions, and the CSS 3D board models are data inside `web/src/build.py`. Commit the regenerated files with your source change. More detail in [`web/README.md`](https://github.com/oauramos/claude-usage-stick/blob/main/web/README.md).

## Adding a board

Roughly what's involved:

1. **`platformio.ini`** — a new env with the board's platform, display driver flags, and pin mapping.
2. **`src/hal.cpp`** — a branch for the board's display init, buttons, battery ADC, and backlight, behind a `BOARD_*` define.
3. **`src/ui.cpp`** — usually nothing, if the board fits an existing [display tier](The-UI#display-tiers). A new resolution class means a new tier.
4. **`wiki/`** — a board guide page, plus a row in [Supported boards](Supported-Boards).
5. **`web/src/build.py`** — an entry in `BOARDS` and a 3D model in `BOARDS3D`, so the board appears in the flasher.
6. **`.github/workflows/pages.yml`** — add the env to the build matrix.

Pin mappings should be verified on real hardware before being merged — several boards in this repo have pin maps that only match after checking against the vendor's own driver.

## Contributing

PRs are welcome, especially board support and photos of real builds. Open an [issue](https://github.com/oauramos/claude-usage-stick/issues) first if you're planning something large.
