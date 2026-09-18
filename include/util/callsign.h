#pragma once

/**
 * Callsign helpers.
 *
 * An airline callsign is three ICAO letters followed by a flight number, as in
 * "DLH4AB". Free of Arduino headers so the host test suite can cover it.
 */
namespace util::callsign {

/**
 * Extract the uppercase three-letter airline prefix from a callsign.
 *
 * Writes four bytes (prefix plus terminator) and returns true only for the
 * airline shape: three letters followed by at least one alphanumeric character.
 * Registrations ("D-EABC"), bare hex ids and tail numbers are rejected, so the
 * radar keeps showing them verbatim instead of inventing an airline.
 */
bool icaoPrefix(const char* text, char out[4]);

}  // namespace util::callsign
