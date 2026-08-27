# What does this PR do?

<!-- Short description of the change. -->

## 📸 Device photo

<!-- REQUIRED for new boards or hardware-related changes:
     attach a photo of YOUR device running this firmware
     (boot screen, PIN screen, or dashboard).
     Just drag & drop the image here. -->

## ✅ Checklist

### All PRs

- [ ] Tested on real hardware — board: <!-- e.g. M5StickC Plus -->

### New board port

- [ ] 📸 Photo of the device **running this firmware** is attached above
- [ ] New `[env:...]` added to `platformio.ini`
- [ ] Board guide added at `wiki/<Board-Name>.md` (specs, flash commands, controls, quirks) — copy an existing page as a template
- [ ] `wiki/Supported-Boards.md` table updated (name, MCU, display, firmware version, PlatformIO env, buy link)
- [ ] `wiki/_Sidebar.md` links to the new guide
- [ ] Board added to `BOARDS` and `BOARDS3D` in `web/src/build.py`, then `python3 web/src/build.py` re-run
- [ ] Env added to the build matrix in `.github/workflows/pages.yml`
