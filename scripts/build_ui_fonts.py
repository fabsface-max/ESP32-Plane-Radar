#!/usr/bin/env python3
"""Build the embedded VLW UI fonts — one file per pixel size.

LovyanGFX maps every VLW glyph pixel to a (size_x, size_y) rectangle, so a font
drawn below its native size loses whole rows and columns of its anti-aliasing
(VLWfont::drawChar). Shipping one file per UI size lets each label render at
scale 1.0 and stay smooth.

VLW layout, all fields big-endian uint32:
  header  gCount, version, size, 0, ascent, descent                (24 bytes)
  glyphs  gCount * (unicode, height, width, xAdvance, dY, dX, 0)   (28 bytes each)
  bitmaps concatenated width*height 8-bit alpha maps, glyph order
Glyph records must be sorted by code point — the renderer binary-searches them.

Requires freetype-py:  pip install freetype-py

Usage: python3 scripts/build_ui_fonts.py
"""

from __future__ import annotations

import struct
import urllib.request
from pathlib import Path

import freetype

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "data"

FONT_URL = (
    "https://raw.githubusercontent.com/googlefonts/noto-fonts/main/hinted/ttf/"
    "NotoSans/NotoSans-SemiBold.ttf"
)
CACHED_TTF = OUT_DIR / ".noto-sans-semibold.ttf"

# Must match kUiFontPx in include/hardware/display_font.h.
SIZES_PX = (15, 13, 11, 9)

# Space plus printable ASCII — everything the radar UI draws.
CODEPOINTS = range(0x20, 0x7F)

HEADER_FMT = ">6I"
GLYPH_FMT = ">7i"
VLW_VERSION = 11


def fetch_ttf() -> Path:
    if not CACHED_TTF.exists():
        with urllib.request.urlopen(FONT_URL, timeout=60) as resp:
            CACHED_TTF.write_bytes(resp.read())
    return CACHED_TTF


def render(ttf: Path, px: int) -> tuple[list[dict], int, int]:
    face = freetype.Face(str(ttf))
    face.set_pixel_sizes(0, px)
    glyphs = []
    for code in CODEPOINTS:
        if face.get_char_index(code) == 0:
            continue
        # The auto-hinter keeps stems on whole pixels, which is what makes the
        # small sizes crisp rather than blurry.
        face.load_char(code, freetype.FT_LOAD_RENDER |
                       freetype.FT_LOAD_TARGET_NORMAL |
                       freetype.FT_LOAD_FORCE_AUTOHINT)
        bitmap = face.glyph.bitmap
        width, height = bitmap.width, bitmap.rows
        # FreeType rows are padded to `pitch`; VLW wants them tightly packed.
        alpha = bytearray(width * height)
        for row in range(height):
            src = row * bitmap.pitch
            alpha[row * width:(row + 1) * width] = bytes(
                bitmap.buffer[src:src + width])
        glyphs.append({
            "unicode": code,
            "width": width,
            "height": height,
            "xadvance": (face.glyph.advance.x + 32) >> 6,
            "dy": face.glyph.bitmap_top,
            "dx": face.glyph.bitmap_left,
            "alpha": alpha,
        })

    ascent = max(g["dy"] for g in glyphs)
    descent = max(g["height"] - g["dy"] for g in glyphs)
    return glyphs, ascent, descent


def write_vlw(path: Path, glyphs: list[dict], px: int, ascent: int,
              descent: int) -> int:
    glyphs = sorted(glyphs, key=lambda g: g["unicode"])
    out = bytearray(struct.pack(HEADER_FMT, len(glyphs), VLW_VERSION, px, 0,
                                ascent, descent))
    for g in glyphs:
        out += struct.pack(GLYPH_FMT, g["unicode"], g["height"], g["width"],
                           g["xadvance"], g["dy"], g["dx"], 0)
    for g in glyphs:
        out += bytes(g["alpha"])
    path.write_bytes(out)
    return len(out)


def main() -> int:
    ttf = fetch_ttf()
    total = 0
    for px in SIZES_PX:
        glyphs, ascent, descent = render(ttf, px)
        path = OUT_DIR / f"ui_font_{px}.vlw"
        size = write_vlw(path, glyphs, px, ascent, descent)
        total += size
        cap = next(g["dy"] for g in glyphs if g["unicode"] == ord("N"))
        print(f"{path.relative_to(ROOT)}: {len(glyphs)} glyphs, "
              f"cap height {cap}px, line height {ascent + descent}px, "
              f"{size} bytes")
    print(f"total {total} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
