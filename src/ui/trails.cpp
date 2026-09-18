#include "ui/trails.h"

#include <cmath>
#include <cstring>

namespace ui::trails {

namespace {

/** A track this old is stale; its slot can be reused. */
constexpr unsigned long kExpiryMs = 90000;
/** Below this the aircraft has not really moved and would just stack points. */
constexpr float kMinStepDeg = 0.0004f;  // ~40 m

struct Track {
  char hex[7];
  uint8_t count;
  uint8_t head;  // next write position in the ring
  unsigned long last_ms;
  Point points[kMaxPoints];
};

Track s_tracks[services::adsb::kMaxAircraft];

Track* findTrack(const char* hex) {
  for (Track& track : s_tracks) {
    if (track.hex[0] != '\0' && std::strcmp(track.hex, hex) == 0) {
      return &track;
    }
  }
  return nullptr;
}

Track* claimTrack(const char* hex, unsigned long now_ms) {
  Track* oldest = nullptr;
  for (Track& track : s_tracks) {
    if (track.hex[0] == '\0') {
      oldest = &track;
      break;
    }
    if (oldest == nullptr || track.last_ms < oldest->last_ms) {
      oldest = &track;
    }
  }
  if (oldest == nullptr) {
    return nullptr;
  }
  *oldest = Track{};
  std::strncpy(oldest->hex, hex, sizeof(oldest->hex) - 1);
  oldest->last_ms = now_ms;
  return oldest;
}

void expire(unsigned long now_ms) {
  for (Track& track : s_tracks) {
    if (track.hex[0] != '\0' && now_ms - track.last_ms > kExpiryMs) {
      track = Track{};
    }
  }
}

void append(Track* track, float lat, float lon) {
  if (track->count > 0) {
    const size_t last =
        (track->head + kMaxPoints - 1) % kMaxPoints;
    const float dlat = track->points[last].lat - lat;
    const float dlon = track->points[last].lon - lon;
    if (std::fabs(dlat) < kMinStepDeg && std::fabs(dlon) < kMinStepDeg) {
      return;
    }
  }
  track->points[track->head] = {lat, lon};
  track->head = static_cast<uint8_t>((track->head + 1) % kMaxPoints);
  if (track->count < kMaxPoints) {
    ++track->count;
  }
}

}  // namespace

void update(const services::adsb::Aircraft* list, size_t count,
            unsigned long now_ms) {
  expire(now_ms);
  if (list == nullptr) {
    return;
  }

  for (size_t i = 0; i < count; ++i) {
    const services::adsb::Aircraft& plane = list[i];
    if (plane.hex[0] == '\0') {
      continue;
    }
    Track* track = findTrack(plane.hex);
    if (track == nullptr) {
      track = claimTrack(plane.hex, now_ms);
    }
    if (track == nullptr) {
      continue;
    }
    track->last_ms = now_ms;
    append(track, plane.lat, plane.lon);
  }
}

size_t historyFor(const char* hex, Point* out, size_t max) {
  if (hex == nullptr || out == nullptr || max == 0 || hex[0] == '\0') {
    return 0;
  }
  const Track* track = findTrack(hex);
  if (track == nullptr || track->count == 0) {
    return 0;
  }

  // The newest entry is the aircraft's current position, which the symbol
  // already marks; the tail is everything before it.
  const size_t available = track->count - 1;
  const size_t wanted = available < max ? available : max;
  const size_t start = (track->head + kMaxPoints - track->count) % kMaxPoints;
  for (size_t i = 0; i < wanted; ++i) {
    out[i] = track->points[(start + (available - wanted) + i) % kMaxPoints];
  }
  return wanted;
}

void clear() {
  for (Track& track : s_tracks) {
    track = Track{};
  }
}

}  // namespace ui::trails
