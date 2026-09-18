#pragma once

namespace data::airlines {

/**
 * Airline name for a callsign like "DLH4AB", or nullptr when the callsign is
 * not in airline shape or the code is unknown. The radar then keeps showing
 * the callsign verbatim.
 */
const char* nameForCallsign(const char* callsign);

}  // namespace data::airlines
