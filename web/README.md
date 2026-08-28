# Web flasher

The page at **https://oauramos.github.io/claude-usage-stick/** — flash any supported board
from Chrome or Edge over Web Serial, no toolchain required.

## Layout

```
web/
  index.html            — generated; edit src/template.html instead
  manifests/<env>.json  — generated; one esp-web-tools manifest per board
  assets/               — photos and the dashboard screenshot used by the page
  firmware/             — published by CI, git-ignored locally
    <env>/{bootloader,partitions,boot_app0,firmware}.bin
    available.json      — envs that built successfully; the page grays out the rest
  src/
    template.html       — the page source (HTML + CSS + JS)
    build.py            — generator: fills in board cards, manifests, the head
```

## Editing

```bash
python3 web/src/build.py    # regenerates index.html + manifests/
```

Board list, firmware versions, and the 3D board models all live in `build.py`; everything
else is in `template.html`. Commit the regenerated `index.html` and `manifests/` together
with your source change.

## Testing locally

Web Serial needs a secure context — `localhost` counts, so a plain static server works:

```bash
# from the repo root, after a `pio run -e <env>` so there is something to flash
mkdir -p web/firmware/<env>
cp .pio/build/<env>/{bootloader,partitions,firmware}.bin web/firmware/<env>/
cp ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin web/firmware/<env>/
echo '["<env>"]' > web/firmware/available.json

python3 -m http.server 8000 --directory web
```

Then open `http://localhost:8000`.

## Deployment

`.github/workflows/pages.yml` runs on every push to `main`: it builds all eight PlatformIO
envs in parallel, collects the four flash images per board, and deploys `web/` to GitHub
Pages with the firmware alongside. A board whose build fails does not block the deploy —
it just doesn't appear in `available.json`, and the page labels its card "build pending".

Enable it once under **Settings → Pages → Source: GitHub Actions**.

## The README's animated hero

`assets/hero.gif` in the repo root is this page's hero section, captured with the
3D board driven one frame per step — GitHub won't run the page's JavaScript, so the
turntable has to be baked in. To regenerate it after a hero change, serve `web/`
locally and, in a Playwright session:

1. inject `* { animation: none !important }` (base styles are the resting state) and
   pin the usage bars' widths, which rest at 0;
2. clone-and-replace the node the site's `requestAnimationFrame` loop rotates, so it
   stops fighting you, then set `transform: rotateY(<angle>deg)` yourself;
3. screenshot 48 angles across 360° with `page.screenshot({clip})` — the hero's own
   rect widened by ~44 px on each side. Pad horizontally only: the site header sits
   flush on top of the hero and the chips row flush below it, so vertical padding
   slices both in half;
4. `ffmpeg -framerate 12 -i f%03d.png … palettegen/paletteuse -loop 0` → ~570 KB at
   900 px wide. GIFs autoplay and loop on their own; there is no parameter for it.

## Flash offsets

Manifests are generated from the offsets PlatformIO uses (`ESP32_APP_OFFSET` and
`FLASH_EXTRA_IMAGES`). The bootloader is the only one that varies by chip: `0x1000` on
ESP32 and ESP32-S2, `0x0` on ESP32-S3 and ESP32-C3.
