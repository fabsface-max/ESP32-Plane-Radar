#pragma once

#include <cstdint>

#include "services/adsb_client.h"

/**
 * Decides when the radar should flash for a noteworthy aircraft.
 *
 * State only — the animation itself is drawn in radar_display, which owns the
 * frame sprite. An aircraft fires once per visit: it has to leave and come back
 * before it can interrupt the display again.
 */
namespace ui::alert {

enum class Kind : uint8_t {
  kNone = 0,
  kNotable = 1,    // A380, 747, outsize freighters
  kMilitary = 2,
  kEmergency = 3,  // squawk 7500/7600/7700 or a declared emergency
};

/** Scan a sweep and arm a flash if something new deserves one. */
void scan(const services::adsb::Aircraft* list, size_t count,
          unsigned long now_ms);

/** kNone when nothing is armed. Cleared by consume(). */
Kind pending();

/** Take the armed alert, leaving nothing behind. */
Kind consume();

/** Forget which aircraft have already fired (e.g. after a settings change). */
void reset();

}  // namespace ui::alert
