#!/usr/bin/env python3
"""Generate the web flasher page (web/index.html) and the per-board manifests.

Run from the repo root after editing template.html:

    python3 web/src/build.py

Assets referenced by the page live in web/assets/; firmware binaries are published
to web/firmware/<env>/ by the CI workflow (.github/workflows/pages.yml), which also
writes web/firmware/available.json — the list the page uses to gray out boards whose
build has not landed yet.
"""
import json, pathlib, urllib.parse

SRC = pathlib.Path(__file__).parent          # web/src
WEB = SRC.parent                             # web

def el(cls, x, y, w, h, extra=""):
    return f'<i class="{cls}" style="left:{x}px;top:{y}px;width:{w}px;height:{h}px;{extra}"></i>'

def pdots(x, y, n, s=4, gap=8):
    return "".join(el("pdot", x + i * gap, y, s, s - 1) for i in range(n))

def pins(x, ys):
    return "".join(el("pin", x, y, 3, 3) for y in ys)

# Each board is a CSS cuboid: front face (screen lit up), back face (module/antenna),
# and --w/--h/--d/--pcb/--pcbe vars for size and material. Sizes are px on the face.
BOARDS3D = {
    "m5stick-cplus": dict(
        w=46, h=110, d=16, pcb="#e8562a", pcbe="#b53f1a",
        front=(el("scr", 8, 12, 30, 50)
               + el("bar", 12, 20, 20, 4) + el("bar d2", 12, 28, 13, 4)
               + pdots(12, 48, 3, 4, 7)
               + el("rbtn", 10, 72, 26, 16) + el("usbp", 15, 103, 16, 5)),
        back=el("plate", 8, 38, 30, 34) + el("usbp", 15, 103, 16, 5),
    ),
    "m5stick-cplus2": dict(
        w=46, h=110, d=16, pcb="#e8562a", pcbe="#b53f1a",
        front=(el("scr", 8, 12, 30, 50)
               + el("bar", 12, 20, 20, 4) + el("bar d2", 12, 28, 13, 4)
               + pdots(12, 48, 3, 4, 7)
               + el("rbtn", 10, 72, 26, 16) + el("rbtn", 41, 40, 4, 13)
               + el("usbp", 15, 103, 16, 5)),
        back=el("plate", 8, 38, 30, 34) + el("usbp", 15, 103, 16, 5),
    ),
    "tdisplay-s3": dict(
        w=150, h=62, d=8, pcb="#26221c", pcbe="#17130e",
        front=(el("scr", 24, 8, 104, 46)
               + el("bar", 32, 16, 62, 5) + el("bar d2", 32, 26, 40, 5)
               + pdots(32, 42, 4, 5, 9)
               + el("rbtn gr", 7, 14, 8, 8) + el("rbtn gr", 7, 40, 8, 8)
               + el("usbp", 143, 24, 6, 14)),
        back=(el("shield", 56, 12, 44, 38) + el("antz", 6, 10, 26, 12)
              + pins(140, [10, 20, 32, 44])),
    ),
    "t8-s2": dict(
        w=48, h=110, d=8, pcb="#26221c", pcbe="#17130e",
        front=(el("scr", 8, 20, 32, 56)
               + el("bar", 13, 30, 22, 4) + el("bar d2", 13, 38, 14, 4)
               + pdots(13, 62, 3, 4, 8)
               + el("rbtn gr", 20, 6, 8, 8) + el("usbp", 16, 102, 16, 5)),
        back=(el("shield", 10, 30, 28, 34) + el("antz", 10, 8, 28, 10)
              + el("usbp", 16, 102, 16, 5)),
    ),
    "crowpanel-adv-35": dict(
        w=150, h=100, d=12, pcb="#2b2620", pcbe="#191510",
        front=(el("scr", 10, 8, 130, 84)
               + el("bar", 24, 26, 88, 7) + el("bar d2", 24, 42, 58, 7)
               + pdots(24, 72, 4, 6, 12)),
        back=(el("shield", 55, 35, 40, 30) + el("antz", 12, 12, 26, 10)
              + el("usbp", 4, 43, 8, 14) + pins(140, [20, 34, 48, 62, 76])),
    ),
    "tdisplay-s3-amoled": dict(
        w=160, h=44, d=8, pcb="#26221c", pcbe="#17130e",
        front=(el("scr", 26, 6, 110, 32)
               + el("bar", 34, 14, 80, 4) + el("bar d2", 34, 23, 50, 4)
               + el("rbtn gr", 148, 10, 6, 6) + el("rbtn gr", 148, 28, 6, 6)
               + el("usbp", 3, 15, 6, 14)),
        back=(el("shield", 62, 8, 44, 28) + el("antz", 20, 12, 26, 10)
              + el("usbp", 3, 15, 6, 14)),
    ),
    "tdisplay-esp32": dict(
        w=48, h=110, d=8, pcb="#26221c", pcbe="#17130e",
        front=(el("scr", 6, 14, 36, 60)
               + el("bar", 11, 26, 26, 4) + el("bar d2", 11, 34, 16, 4)
               + pdots(11, 58, 3, 4, 8)
               + el("rbtn sq", 6, 92, 16, 10) + el("rbtn sq", 26, 92, 16, 10)
               + el("usbp", 16, 104, 16, 5)),
        back=(el("shield", 10, 28, 28, 36) + el("antz", 10, 6, 28, 10)
              + el("usbp", 16, 104, 16, 5)),
    ),
    "esp32c3-oled": dict(
        w=42, h=62, d=8, pcb="#26221c", pcbe="#17130e",
        front=(el("scr", 7, 10, 28, 16)
               + el("bar", 10, 14, 17, 3) + el("bar d2", 10, 19, 10, 3)
               + pins(3, [32, 39, 46, 53]) + pins(36, [32, 39, 46, 53])
               + el("usbp", 11, 53, 20, 7)),
        back=el("shield", 9, 24, 24, 22) + el("usbp", 11, 53, 20, 7),
    ),
}

BOARDS = [
    # env, name, mcu, display, firmware tag (label, is_mango), chip family, fw version
    ("m5stick-cplus", "M5StickC Plus", "ESP32-PICO", '1.14&Prime; 240&times;135', ("&#129389; Mango &middot; tier S", True), "ESP32", "2.0.0"),
    ("m5stick-cplus2", "M5StickC Plus2", "ESP32-PICO-V3", '1.14&Prime; 240&times;135', ("Clarity v1", False), "ESP32", "1.0.0"),
    ("tdisplay-s3", "LilyGo T-Display S3", "ESP32-S3", '1.9&Prime; 320&times;170', ("&#129389; Mango &middot; tier L", True), "ESP32-S3", "2.1.1"),
    ("t8-s2", "LilyGo T8 ESP32-S2", "ESP32-S2", '1.14&Prime; 135&times;240', ("Clarity v1", False), "ESP32-S2", "1.0.0"),
    ("crowpanel-adv-35", "CrowPanel Advance 3.5&Prime;", "ESP32-S3", '3.5&Prime; 480&times;320 touch', ("Clarity v1", False), "ESP32-S3", "1.0.0"),
    ("tdisplay-s3-amoled", "T-Display S3 AMOLED", "ESP32-S3", '1.91&Prime; 240&times;536', ("Clarity v1", False), "ESP32-S3", "1.0.0"),
    ("tdisplay-esp32", "TTGO T-Display", "ESP32", '1.14&Prime; 135&times;240', ("Clarity v1", False), "ESP32", "1.0.0"),
    ("esp32c3-oled", "ESP32-C3-OLED", "ESP32-C3", '0.42&Prime; 72&times;40 OLED', ("Clarity v1", False), "ESP32-C3", "1.0.0"),
]

CLAWD_RECTS = [
    (72, 91, 258, 184, "#f25a45"), (12, 152, 60, 62, "#f25a45"), (330, 152, 58, 62, "#f25a45"),
    (72, 275, 34, 68, "#f25a45"), (137, 275, 32, 68, "#f25a45"), (231, 275, 32, 68, "#f25a45"),
    (294, 275, 36, 68, "#f25a45"),
    (107, 120, 33, 31, "#0e0d0d"), (262, 120, 32, 31, "#0e0d0d"),
]

def clawd_favicon():
    rects = "".join(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' fill='{c}'/>" for x, y, w, h, c in CLAWD_RECTS)
    svg = f"<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 400 400'>{rects}</svg>"
    return "data:image/svg+xml," + urllib.parse.quote(svg)

def model3d(env):
    b = BOARDS3D[env]
    vars_ = f"--w:{b['w']}px;--h:{b['h']}px;--d:{b['d']}px;--pcb:{b['pcb']};--pcbe:{b['pcbe']}"
    return f'''<span class="b3d-stage" style="{vars_}">
      <span class="b3d-float"><span class="b3d-tilt"><span class="b3d">
        <span class="face f">{b['front']}</span>
        <span class="face k">{b['back']}</span>
        <span class="edge l"></span><span class="edge r"></span>
        <span class="edge t"></span><span class="edge b"></span>
      </span></span></span>
      <span class="b3d-glow"></span>
    </span>'''

def card(env, name, mcu, display, fw, chip, _version):
    label, mango = fw
    fw_cls = "tag mango" if mango else "tag"
    return f'''<div class="board-card">
  <input type="radio" name="board" id="b-{env}" value="{env}">
  <label for="b-{env}">
    <span class="board-illo">{model3d(env)}</span>
    <h4>{name}</h4>
    <p class="board-meta">{mcu} &middot; {display}</p>
    <span class="board-tags"><span class="{fw_cls}">{label}</span><span class="tag chipfam">{chip}</span></span>
  </label>
  <span class="sel-dot"></span>
</div>'''

def hero3d(screen_url):
    """Big T-Display S3 in a molded case, with the real Mango tier-L screenshot on screen."""
    front = (f'<i class="scr scr-img" style="left:48px;top:8px;width:290px;height:154px;'
             f'background-image:url({screen_url})"></i>'
             + el("rbtn gr", 14, 38, 18, 18) + el("rbtn gr", 14, 114, 18, 18))
    back = (el("grille", 50, 52, 270, 66)
            + el("screw", 12, 12, 8, 8) + el("screw", 350, 12, 8, 8)
            + el("screw", 12, 150, 8, 8) + el("screw", 350, 150, 8, 8))
    usb_port = el("usbc", 12, 65, 12, 40)
    return f'''<div class="b3d-stage hero-stage" data-speed="26" style="--w:370px;--h:170px;--d:36px;--pcb:#221d18;--pcbe:#1a1612">
      <span class="b3d-scale"><span class="b3d-float"><span class="b3d-tilt"><span class="b3d b3d-case">
        <span class="face f">{front}</span>
        <span class="face k">{back}</span>
        <span class="edge l"></span><span class="edge r">{usb_port}</span>
        <span class="edge t"></span><span class="edge b"></span>
      </span></span></span></span>
      <span class="b3d-glow"></span>
    </div>'''



# ── production-only transforms ────────────────────────────────────────────────

ESP_SCRIPT = ('<script type="module" '
              'src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>')

MOCK_ACTIONS = '''    <div class="dlg-actions">
      <button class="btn-primary" id="dlgConnect">Connect &amp; flash</button>
      <button class="btn-ghost" id="dlgCancel">Cancel</button>
    </div>
    <p class="dlg-preview-note" id="previewNote">🛠️ Design preview — live Web Serial flashing is wired up in the GitHub Pages build, where this button becomes the real <span style="font-family:var(--font-mono)">esp-web-install-button</span>.</p>'''

REAL_ACTIONS = '''    <div class="dlg-actions">
      <esp-web-install-button id="espInstall">
        <button class="btn-primary" slot="activate">Connect &amp; flash</button>
        <span class="dlg-unsup" slot="unsupported">Web Serial isn't available in this browser — use Chrome or Edge on desktop.</span>
        <span class="dlg-unsup" slot="not-allowed">Flashing needs a secure (HTTPS) page.</span>
      </esp-web-install-button>
      <button class="btn-ghost" id="dlgCancel">Cancel</button>
    </div>'''

MOCK_NOTE_CSS = '''  .dlg-preview-note {
    margin-top: 16px; font-size: 13px; color: var(--text-faint);
    border-top: 1px dashed var(--line); padding-top: 14px; display: none;
  }
  .dlg-preview-note.show { display: block; }'''

REAL_NOTE_CSS = '''  .dlg-actions esp-web-install-button { flex: 1; display: block; }
  .dlg-actions esp-web-install-button .btn-primary { width: 100%; justify-content: center; font-size: 15px; padding: 13px; }
  .dlg-unsup { font-family: var(--font-mono); font-size: 13px; color: var(--text-faint); display: block; padding: 12px 0; }
  .board-card.unavailable label { opacity: .45; cursor: not-allowed; }
  .board-card.unavailable label:hover { transform: none; border-color: var(--line-soft); }
  .board-card .pending-tag {
    position: absolute; top: 10px; left: 10px; z-index: 2; font-family: var(--font-mono);
    font-size: 10px; color: var(--mango); border: 1px solid rgba(240,168,60,.4);
    border-radius: 5px; padding: 2px 6px; background: var(--screen);
  }'''

MOCK_JS_REFS = '''  const dlgBoard = document.getElementById("dlgBoard");
  const previewNote = document.getElementById("previewNote");'''

REAL_JS_REFS = '''  const dlgBoard = document.getElementById("dlgBoard");
  const espInstall = document.getElementById("espInstall");'''

MOCK_JS_SELECT = '''      selected = r.value;
      btn.disabled = false;
      btnLabel.textContent = "Flash " + BOARDS[selected].name;'''

REAL_JS_SELECT = '''      selected = r.value;
      btn.disabled = false;
      btnLabel.textContent = "Flash " + BOARDS[selected].name;
      espInstall.setAttribute("manifest", "manifests/" + selected + ".json");'''

MOCK_JS_OPEN = '''    dlgBoard.textContent = b.name + " · " + b.chip + " · env " + selected;
    previewNote.classList.remove("show");
    dlg.showModal();'''

REAL_JS_OPEN = '''    dlgBoard.textContent = b.name + " · " + b.chip + " · env " + selected;
    dlg.showModal();'''

MOCK_JS_CONNECT = '''  document.getElementById("dlgCancel").addEventListener("click", () => dlg.close());
  document.getElementById("dlgConnect").addEventListener("click", () => previewNote.classList.add("show"));'''

REAL_JS_CONNECT = '''  document.getElementById("dlgCancel").addEventListener("click", () => dlg.close());

  // gray out boards whose CI firmware isn't published yet
  fetch("firmware/available.json")
    .then(r => (r.ok ? r.json() : null))
    .then(list => {
      if (!Array.isArray(list)) return;
      document.querySelectorAll('input[name="board"]').forEach(r => {
        if (list.includes(r.value)) return;
        r.disabled = true;
        const card = r.closest(".board-card");
        card.classList.add("unavailable");
        const tag = document.createElement("span");
        tag.className = "pending-tag";
        tag.textContent = "build pending";
        card.appendChild(tag);
      });
    })
    .catch(() => {});'''

HEAD_TOP = '''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="description" content="Flash the Claude Usage Stick firmware to your ESP32 board straight from the browser — no toolchain, no drivers, no cloned repo.">
<link rel="icon" type="image/svg+xml" href="FAVICON">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
'''

# bootloader flash offset per chip family (partitions/boot_app0/app are the same everywhere)
BOOTLOADER_OFFSET = {"ESP32": 0x1000, "ESP32-S2": 0x1000, "ESP32-S3": 0x0, "ESP32-C3": 0x0}

def manifest(env, name, chip, version):
    boot = BOOTLOADER_OFFSET[chip]
    return {
        "name": f"Claude Usage Stick — {name}",
        "version": version,
        "new_install_prompt_erase": True,
        "builds": [{
            "chipFamily": chip,
            "parts": [
                {"path": f"../firmware/{env}/bootloader.bin", "offset": boot},
                {"path": f"../firmware/{env}/partitions.bin", "offset": 0x8000},
                {"path": f"../firmware/{env}/boot_app0.bin", "offset": 0xE000},
                {"path": f"../firmware/{env}/firmware.bin", "offset": 0x10000},
            ],
        }],
    }

def build():
    html = (SRC / "template.html").read_text()
    html = html.replace("<!-- BOARD_CARDS -->", "\n".join(card(*b) for b in BOARDS))

    # production: file assets, real flashing, full HTML document
    web = WEB
    (web / "manifests").mkdir(parents=True, exist_ok=True)

    html = html.replace("<!-- HERO_3D -->", hero3d("assets/tds3-screen.png"))

    for mock, real in [(MOCK_ACTIONS, REAL_ACTIONS), (MOCK_NOTE_CSS, REAL_NOTE_CSS),
                       (MOCK_JS_REFS, REAL_JS_REFS), (MOCK_JS_SELECT, REAL_JS_SELECT),
                       (MOCK_JS_OPEN, REAL_JS_OPEN), (MOCK_JS_CONNECT, REAL_JS_CONNECT)]:
        assert mock in html, f"anchor not found:\n{mock[:80]}"
        html = html.replace(mock, real)

    # lift the <title> + font link into a proper <head>, append esp-web-tools loader
    title_line = '<title>Claude Usage Stick Flasher</title>'
    fonts_line = next(l for l in html.splitlines() if "fonts.googleapis.com/css2" in l)
    body = html.replace(title_line + "\n", "").replace(fonts_line + "\n", "")
    head = (HEAD_TOP.replace("FAVICON", clawd_favicon())
            + title_line + "\n" + fonts_line + "\n" + ESP_SCRIPT + "\n</head>\n<body>\n")
    html = head + body + "\n</body>\n</html>\n"

    (web / "index.html").write_text(html)
    for env, name, _mcu, _disp, _fw, chip, version in BOARDS:
        (web / "manifests" / f"{env}.json").write_text(json.dumps(manifest(env, name, chip, version), indent=2) + "\n")
    print(f"wrote {web / 'index.html'} ({len(html)/1024:.0f} KB) + {len(BOARDS)} manifests")

if __name__ == "__main__":
    build()
