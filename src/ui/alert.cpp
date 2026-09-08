#include "ui/alert.h"

#include <cstring>

#include "util/aircraft_class.h"

namespace ui::alert {

namespace {

/** How many aircraft are remembered as "already announced". */
constexpr size_t kSeenSlots = 24;
/** After this long out of range, an aircraft may announce itself again. */
constexpr unsigned long kSeenExpiryMs = 600000;  // 10 minutes

struct Seen {
  char hex[7];
  unsigned long last_ms;
};

Seen s_seen[kSeenSlots];
Kind s_pending = Kind::kNone;

Kind kindFor(uint8_t flags) {
  // Ordered by how much it deserves to interrupt what you were looking at.
  if ((flags & util::aircraft::kFlagEmergency) != 0) {
    return Kind::kEmergency;
  }
  if ((flags & util::aircraft::kFlagMilitary) != 0) {
    return Kind::kMilitary;
  }
  if ((flags & util::aircraft::kFlagNotable) != 0) {
    return Kind::kNotable;
  }
  return Kind::kNone;
}

/** True when this aircraft has already announced itself recently. */
bool alreadySeen(const char* hex, unsigned long now_ms) {
  for (Seen& slot : s_seen) {
    if (slot.hex[0] != '\0' && std::strcmp(slot.hex, hex) == 0) {
      if (now_ms - slot.last_ms > kSeenExpiryMs) {
        slot.last_ms = now_ms;
        return false;
      }
      slot.last_ms = now_ms;
      return true;
    }
  }
  return false;
}

void remember(const char* hex, unsigned long now_ms) {
  Seen* target = nullptr;
  for (Seen& slot : s_seen) {
    if (slot.hex[0] == '\0') {
      target = &slot;
      break;
    }
    if (target == nullptr || slot.last_ms < target->last_ms) {
      target = &slot;
    }
  }
  if (target == nullptr) {
    return;
  }
  std::memset(target, 0, sizeof(*target));
  std::strncpy(target->hex, hex, sizeof(target->hex) - 1);
  target->last_ms = now_ms;
}

}  // namespace

void scan(const services::adsb::Aircraft* list, size_t count,
          unsigned long now_ms) {
  if (list == nullptr) {
    return;
  }

  for (size_t i = 0; i < count; ++i) {
    const services::adsb::Aircraft& plane = list[i];
    const Kind kind = kindFor(plane.alert_flags);
    if (kind == Kind::kNone || plane.hex[0] == '\0') {
      continue;
    }
    if (alreadySeen(plane.hex, now_ms)) {
      continue;
    }
    remember(plane.hex, now_ms);
    // Keep the most urgent of everything new in this sweep.
    if (static_cast<uint8_t>(kind) > static_cast<uint8_t>(s_pending)) {
      s_pending = kind;
    }
  }
}

Kind pending() { return s_pending; }

Kind consume() {
  const Kind kind = s_pending;
  s_pending = Kind::kNone;
  return kind;
}

void reset() {
  std::memset(s_seen, 0, sizeof(s_seen));
  s_pending = Kind::kNone;
}

}  // namespace ui::alert
