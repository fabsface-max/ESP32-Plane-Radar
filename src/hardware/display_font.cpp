#include "hardware/display_font.h"

#include "hardware/display.h"

extern "C" {
extern const uint8_t _binary_data_ui_font_15_vlw_start[] asm(
    "_binary_data_ui_font_15_vlw_start");
extern const uint8_t _binary_data_ui_font_15_vlw_end[] asm(
    "_binary_data_ui_font_15_vlw_end");
extern const uint8_t _binary_data_ui_font_13_vlw_start[] asm(
    "_binary_data_ui_font_13_vlw_start");
extern const uint8_t _binary_data_ui_font_13_vlw_end[] asm(
    "_binary_data_ui_font_13_vlw_end");
extern const uint8_t _binary_data_ui_font_11_vlw_start[] asm(
    "_binary_data_ui_font_11_vlw_start");
extern const uint8_t _binary_data_ui_font_11_vlw_end[] asm(
    "_binary_data_ui_font_11_vlw_end");
extern const uint8_t _binary_data_ui_font_9_vlw_start[] asm(
    "_binary_data_ui_font_9_vlw_start");
extern const uint8_t _binary_data_ui_font_9_vlw_end[] asm(
    "_binary_data_ui_font_9_vlw_end");
}

const uint8_t kUiFontPx[kUiFontCount] = {15, 13, 11, 9};

namespace {

struct EmbeddedFont {
  const uint8_t* start;
  const uint8_t* end;
};

const EmbeddedFont kFonts[kUiFontCount] = {
    {_binary_data_ui_font_15_vlw_start, _binary_data_ui_font_15_vlw_end},
    {_binary_data_ui_font_13_vlw_start, _binary_data_ui_font_13_vlw_end},
    {_binary_data_ui_font_11_vlw_start, _binary_data_ui_font_11_vlw_end},
    {_binary_data_ui_font_9_vlw_start, _binary_data_ui_font_9_vlw_end},
};

bool s_vlw_available = false;

/**
 * Which font each LGFX instance currently holds. Reloading a VLW parses its
 * whole glyph table, and the radar draws up to 64 aircraft tags per frame, so
 * repeat loads have to be cheap no-ops. Two slots cover the panel and the
 * off-screen frame sprite.
 */
constexpr size_t kTrackedInstances = 2;
struct ActiveFont {
  const lgfx::LGFXBase* gfx;
  size_t index;
};
ActiveFont s_active[kTrackedInstances] = {{nullptr, 0}, {nullptr, 0}};

size_t fontLen(size_t index) {
  return static_cast<size_t>(kFonts[index].end - kFonts[index].start);
}

bool vlwActiveOn(const lgfx::LGFXBase& gfx) {
  const lgfx::IFont* font = gfx.getFont();
  return font != nullptr && font->getType() == lgfx::IFont::font_type_t::ft_vlw;
}

ActiveFont* slotFor(const lgfx::LGFXBase& gfx) {
  for (ActiveFont& slot : s_active) {
    if (slot.gfx == &gfx) {
      return &slot;
    }
  }
  for (ActiveFont& slot : s_active) {
    if (slot.gfx == nullptr) {
      slot.gfx = &gfx;
      slot.index = kUiFontCount;  // nothing loaded yet
      return &slot;
    }
  }
  return nullptr;
}

void forgetSlot(const lgfx::LGFXBase& gfx) {
  for (ActiveFont& slot : s_active) {
    if (slot.gfx == &gfx) {
      slot.index = kUiFontCount;
    }
  }
}

}  // namespace

bool displayFontInit() {
  s_vlw_available = true;
  for (size_t i = 0; i < kUiFontCount; ++i) {
    if (fontLen(i) == 0) {
      s_vlw_available = false;
    }
  }
  if (s_vlw_available) {
    s_vlw_available = displayFontEnsureLoaded(tft, kUiFont15);
  }
  if (!s_vlw_available) {
    Serial.println("Smooth fonts unavailable — using bitmap fallback");
  }
  return s_vlw_available;
}

bool displayFontIsSmooth() { return s_vlw_available; }

bool displayFontEnsureLoaded(lgfx::LGFXBase& gfx, size_t index) {
  if (index >= kUiFontCount || fontLen(index) == 0) {
    return false;
  }

  ActiveFont* slot = slotFor(gfx);
  if (slot != nullptr && slot->index == index && vlwActiveOn(gfx)) {
    gfx.setTextSize(1.0f);
    return true;
  }

  if (!gfx.loadFont(kFonts[index].start, lgfx::IFont::font_type_t::ft_vlw)) {
    forgetSlot(gfx);
    return false;
  }
  gfx.setTextSize(1.0f);
  if (slot != nullptr) {
    slot->index = index;
  }
  return true;
}

void displayFontSetSmoothSize(lgfx::LGFXBase& gfx, float size) {
  gfx.setTextSize(size);
}

void displayFontSetBitmap(lgfx::LGFXBase& gfx, const lgfx::GFXfont* font) {
  gfx.setFont(font);
  gfx.setTextSize(1);
  forgetSlot(gfx);
}
