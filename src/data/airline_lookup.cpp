#include "data/airline_lookup.h"

#include <cstring>

#include "data/airlines.h"
#include "util/callsign.h"

namespace data::airlines {

const char* nameForCallsign(const char* callsign) {
  char code[4] = {};
  if (!util::callsign::icaoPrefix(callsign, code)) {
    return nullptr;
  }

  // kAirlines is generated sorted by ICAO code.
  size_t lo = 0;
  size_t hi = kAirlineCount;
  while (lo < hi) {
    const size_t mid = lo + (hi - lo) / 2;
    const int cmp = std::strcmp(code, kAirlines[mid].icao);
    if (cmp == 0) {
      return kAirlines[mid].name;
    }
    if (cmp < 0) {
      hi = mid;
    } else {
      lo = mid + 1;
    }
  }
  return nullptr;
}

}  // namespace data::airlines
