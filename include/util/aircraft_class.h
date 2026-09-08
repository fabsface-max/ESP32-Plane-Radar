#pragma once

#include <cstdint>

/**
 * Classify an aircraft for drawing and for alerts.
 *
 * The ADS-B feed is inconsistent: `category`, `dbFlags`, `emergency` and
 * `squawk` are all optional and many aircraft send none of them. Every rule
 * here therefore degrades to a safe default rather than guessing, and each
 * input is treated as untrusted text. Free of Arduino headers so the host test
 * suite can cover it.
 */
namespace util::aircraft {

/**
 * Drawn size class. On a 240 px display only two silhouettes survive rotation
 * — a triangle and a rotor — so weight is carried by triangle size instead.
 */
enum class Klass : uint8_t {
  kLight = 0,  // small triangle
  kJet = 1,    // the symbol the radar has always drawn
  kHeavy = 2,  // large triangle: A380, 747 and friends
  kRotor = 3,  // rotor disc, heading-independent
};

/** Reasons an aircraft is worth flashing the display for. */
constexpr uint8_t kFlagMilitary = 1u << 0;
constexpr uint8_t kFlagEmergency = 1u << 1;
constexpr uint8_t kFlagNotable = 1u << 2;

/** dbFlags bit 0 marks military traffic in the readsb-derived feeds. */
constexpr uint32_t kDbFlagMilitary = 1u << 0;

/**
 * @param type_code ICAO type designator ("A388", "EC35"), may be empty.
 * @param category  ADS-B emitter category ("A1".."A7"), may be empty.
 */
Klass classify(const char* type_code, const char* category);

/**
 * @param emergency readsb `emergency` string ("none", "general", ...), optional.
 * @param squawk    transponder code as text ("7700"), optional.
 * @param db_flags  readsb `dbFlags` bitfield; 0 when the feed omits it.
 */
uint8_t alertFlags(const char* type_code, const char* emergency,
                   const char* squawk, uint32_t db_flags);

/** True for 7500 (hijack), 7600 (radio failure) and 7700 (general emergency). */
bool isEmergencySquawk(const char* squawk);

}  // namespace util::aircraft
