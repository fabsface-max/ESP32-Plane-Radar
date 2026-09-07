#pragma once

#include <LovyanGFX.hpp>

#include <cstddef>
#include <cstdint>

/**
 * Embedded smooth (VLW) fonts — one file per native pixel size, largest first.
 *
 * LovyanGFX renders a VLW glyph by mapping each source pixel to a
 * (size_x, size_y) rectangle, so a font drawn below its native size loses whole
 * rows and columns of its anti-aliasing. Labels therefore pick the file that
 * matches their target height and draw it at scale 1.0 instead of shrinking one
 * large font. Regenerate with scripts/build_ui_fonts.py.
 */
constexpr size_t kUiFontCount = 4;
constexpr size_t kUiFont15 = 0;
constexpr size_t kUiFont13 = 1;
constexpr size_t kUiFont11 = 2;
constexpr size_t kUiFont9 = 3;

/** Native pixel size of each embedded font, in kUiFont* order. */
extern const uint8_t kUiFontPx[kUiFontCount];

bool displayFontInit();
bool displayFontIsSmooth();

/** Load embedded font `index` on gfx; no-op when it is already the active one. */
bool displayFontEnsureLoaded(lgfx::LGFXBase& gfx, size_t index);

/** VLW: setTextSize scale (1.0 = native pixel size). Bitmap: no-op. */
void displayFontSetSmoothSize(lgfx::LGFXBase& gfx, float size);

/** Bitmap GFXfont fallback; clears any runtime VLW font on this instance. */
void displayFontSetBitmap(lgfx::LGFXBase& gfx, const lgfx::GFXfont* font);
