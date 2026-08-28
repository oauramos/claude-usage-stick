#!/usr/bin/env python3
"""Regenerates src/panel_html.h from web/panel/panel.html.

The page is gzipped (deterministically: mtime=0, level 9) into a PROGMEM byte
array that panel.cpp serves with Content-Encoding: gzip. Run from anywhere:

    python3 web/panel/build.py
"""

import gzip
import hashlib
import io
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
SRC = ROOT / "web" / "panel" / "panel.html"
OUT = ROOT / "src" / "panel_html.h"


def main() -> None:
    html = SRC.read_text(encoding="utf-8")

    # The device serves this on a LAN with no internet guarantee — refuse any
    # external RESOURCE (fonts, scripts, styles, images) that would break the
    # page offline. Plain <a href> links are fine: they only matter on click.
    external = re.findall(r'<(?:link|script|img|iframe)[^>]*(?:src|href)="https?://[^"]+"', html)
    if external:
        sys.exit(f"panel.html loads external resources: {external}")

    buf = io.BytesIO()
    with gzip.GzipFile(fileobj=buf, mode="wb", compresslevel=9, mtime=0) as gz:
        gz.write(html.encode("utf-8"))
    data = buf.getvalue()

    lines = []
    for i in range(0, len(data), 16):
        chunk = ", ".join(f"0x{b:02x}" for b in data[i : i + 16])
        lines.append(f"    {chunk},")
    body = "\n".join(lines)

    # Content-derived ETag so a changed page busts browser caches even when
    # FW_VERSION stays the same between dev reflashes.
    etag = hashlib.sha1(html.encode("utf-8")).hexdigest()[:12]

    OUT.write_text(
        "// GENERATED — do not edit. Source: web/panel/panel.html\n"
        "// Regenerate with: python3 web/panel/build.py\n"
        "#pragma once\n"
        "#include <pgmspace.h>\n"
        "\n"
        f"// {len(html)} bytes raw, {len(data)} bytes gzipped\n"
        f"static const uint8_t PANEL_HTML_GZ[] PROGMEM = {{\n{body}\n}};\n"
        f"static const unsigned int PANEL_HTML_GZ_LEN = {len(data)};\n"
        f'static const char PANEL_HTML_ETAG[] = "\\"{etag}\\"";\n',
        encoding="utf-8",
    )
    print(f"wrote {OUT.relative_to(ROOT)} ({len(html) / 1024:.1f} KB raw → {len(data) / 1024:.1f} KB gz)")


if __name__ == "__main__":
    main()
