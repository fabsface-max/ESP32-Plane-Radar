#include "util/aircraft_class.h"

#include <cstddef>
#include <cstring>

namespace util::aircraft {

namespace {

/**
 * Rotorcraft type designators. ADS-B category A7 is the authoritative signal,
 * but most helicopters transmit no category at all, and a helicopter drawn as
 * an airliner is the one mistake that looks obviously wrong on screen.
 *
 * Matched exactly, never as a prefix: "B29" would otherwise claim the B-29 for
 * Bell, and "B47" the B-47 Stratojet.
 */
const char* const kRotorTypes[] = {
    "A109", "A119", "A139", "A149", "A169", "A189", "AS32", "AS3B", "AS350",
    "AS35", "AS50", "AS55", "AS65", "AW09", "AW89", "B06", "B06T", "B105",
    "B212", "B222", "B230", "B407", "B412", "B427", "B429", "B430", "B47G",
    "B47J", "B505", "BK17", "EC20", "EC25", "EC30", "EC35", "EC45", "EC55",
    "EC75", "EH10", "EN28", "EXPL", "G2CA", "H47", "H500", "H60", "H64",
    "HUCO", "KA26", "KA32", "LYNX", "MD50", "MD52", "MD60", "MI2", "MI8",
    "MI17", "MI24", "MI26", "NH90", "PUMA", "R22", "R44", "R66", "S330",
    "S61", "S64", "S70", "S76", "S92", "TIGR", "UH1", "UH60",
};

/**
 * Wide-body and outsize types, drawn with the large triangle. Kept short on
 * purpose: a false positive here is noisier than a miss.
 */
const char* const kHeavyTypes[] = {
    "A124", "A225", "A306", "A30B", "A310", "A332", "A333", "A337", "A338",
    "A339", "A342", "A343", "A345", "A346", "A359", "A35K", "A388", "A3ST",
    "A400", "AN22", "B741", "B742", "B743", "B744", "B748", "B74D", "B74R",
    "B74S", "B762", "B763", "B764", "B772", "B773", "B77L", "B77W", "B778",
    "B779", "B788", "B789", "B78X", "C5M", "DC10", "IL76", "IL86", "IL96",
    "MD11",
};

/**
 * The ones worth interrupting the display for: the double decker, outsize
 * freighters, and the oddities you would walk to the window for.
 */
const char* const kNotableTypes[] = {
    "A124", "A225", "A337", "A388", "A3ST", "A400", "AN22", "B52", "B703",
    "B741", "B742", "B743", "B744", "B748", "B74D", "B74R", "B74S", "C5M",
    "CONC", "IL76", "IL86", "IL96",
};

template <size_t N>
bool matchesExactly(const char* type_code, const char* const (&table)[N]) {
  if (type_code == nullptr || type_code[0] == '\0') {
    return false;
  }
  for (size_t i = 0; i < N; ++i) {
    if (std::strcmp(type_code, table[i]) == 0) {
      return true;
    }
  }
  return false;
}

/** Emitter category, e.g. "A7" for rotorcraft. Returns 0 when absent. */
char categoryDigit(const char* category) {
  if (category == nullptr || category[0] != 'A' || category[1] == '\0' ||
      category[2] != '\0') {
    return 0;
  }
  return category[1];
}

}  // namespace

Klass classify(const char* type_code, const char* category) {
  const char digit = categoryDigit(category);
  if (digit == '7') {
    return Klass::kRotor;
  }
  if (matchesExactly(type_code, kRotorTypes)) {
    return Klass::kRotor;
  }
  if (digit == '5' || matchesExactly(type_code, kHeavyTypes)) {
    return Klass::kHeavy;
  }
  if (digit == '1') {
    return Klass::kLight;
  }
  // Without a category there is no dependable light/jet split, and the size
  // difference between the two is subtle anyway — default to the jet symbol
  // the radar has always drawn.
  return Klass::kJet;
}

bool isEmergencySquawk(const char* squawk) {
  if (squawk == nullptr) {
    return false;
  }
  return std::strcmp(squawk, "7500") == 0 || std::strcmp(squawk, "7600") == 0 ||
         std::strcmp(squawk, "7700") == 0;
}

uint8_t alertFlags(const char* type_code, const char* emergency,
                   const char* squawk, uint32_t db_flags) {
  uint8_t flags = 0;

  if ((db_flags & kDbFlagMilitary) != 0) {
    flags |= kFlagMilitary;
  }

  // readsb sends "none" for the overwhelming majority; anything else is a
  // declared emergency. The squawk is the fallback for feeds that omit it.
  if (emergency != nullptr && emergency[0] != '\0' &&
      std::strcmp(emergency, "none") != 0) {
    flags |= kFlagEmergency;
  }
  if (isEmergencySquawk(squawk)) {
    flags |= kFlagEmergency;
  }

  if (matchesExactly(type_code, kNotableTypes)) {
    flags |= kFlagNotable;
  }

  return flags;
}

}  // namespace util::aircraft
