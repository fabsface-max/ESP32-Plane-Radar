#pragma once

/**
 * Latitude/longitude parsing for portal input and stored preferences.
 *
 * Free of Arduino headers so the host test suite can cover it
 * (test/test_util). Out-of-range or malformed coordinates must be rejected:
 * they feed the URL of the ADS-B request and the radar's own projection.
 */
namespace util::coords {

constexpr double kLatMin = -90.0;
constexpr double kLatMax = 90.0;
constexpr double kLonMin = -180.0;
constexpr double kLonMax = 180.0;

inline bool validLatLon(double lat, double lon) {
  // NaN fails every comparison, so this rejects it too.
  return lat >= kLatMin && lat <= kLatMax && lon >= kLonMin && lon <= kLonMax;
}

/**
 * Strict decimal parse: the whole string must be one number. Returns false —
 * leaving *out untouched — on empty input, trailing characters, or a value
 * outside the coordinate range.
 */
bool parseCoordinate(const char* text, double* out);

}  // namespace util::coords
