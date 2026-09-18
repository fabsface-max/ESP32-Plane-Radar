#pragma once

#include <cstddef>

#include "services/adsb_client.h"

/**
 * Short position history per aircraft, drawn as a thin grey tail.
 *
 * Tracks are keyed by the ICAO hex id, because the aircraft array is rebuilt
 * from scratch on every fetch and indices mean nothing across sweeps.
 */
namespace ui::trails {

struct Point {
  float lat;
  float lon;
};

/** Roughly half a minute of history at the current fetch interval. */
constexpr size_t kMaxPoints = 8;

/** Record one sweep. Call once per successful fetch, before drawing. */
void update(const services::adsb::Aircraft* list, size_t count,
            unsigned long now_ms);

/**
 * Copy the history for `hex`, oldest first, excluding the current position.
 * Returns the number of points written.
 */
size_t historyFor(const char* hex, Point* out, size_t max);

/** Forget everything, e.g. when the radar centre moves. */
void clear();

}  // namespace ui::trails
