#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

/**
 * Parsers for values arriving from the Wi-Fi setup portal.
 *
 * Everything here is reachable by anyone who can open the portal, so each
 * parser rejects rather than guesses: a malformed field must leave the stored
 * setting untouched. Free of Arduino headers so the host test suite can cover
 * it (test/test_util).
 */
namespace util::portal {

/**
 * A browser submits a checkbox's value= attribute only when it is ticked, and
 * the portal prefills every checkbox with "T"; an unticked box arrives empty.
 * Only affirmative spellings count — treating any non-empty string as "on"
 * would turn a malformed or hostile POST into a silent settings change.
 */
inline bool checkboxChecked(const char* value) {
  if (value == nullptr || value[0] == '\0') {
    return false;
  }
  if (value[1] == '\0') {
    return value[0] == 'T' || value[0] == 't' || value[0] == '1';
  }
  return std::strcmp(value, "on") == 0 || std::strcmp(value, "true") == 0;
}

/**
 * Parse a 1-based step number from a portal number field into a 0-based index.
 * Returns false — leaving *out untouched — for anything that is not a plain
 * integer in 1..count, including trailing characters and overlong input.
 */
inline bool stepIndex(const char* value, uint8_t count, uint8_t* out) {
  if (value == nullptr || out == nullptr || count == 0) {
    return false;
  }

  // Hand-rolled rather than strtol: no locale, no overflow, no sign, and a
  // hard length bound regardless of what the field contained.
  uint32_t parsed = 0;
  size_t digits = 0;
  for (const char* p = value; *p != '\0'; ++p) {
    if (*p < '0' || *p > '9') {
      return false;
    }
    if (++digits > 3) {
      return false;
    }
    parsed = parsed * 10 + static_cast<uint32_t>(*p - '0');
  }

  if (digits == 0 || parsed < 1 || parsed > count) {
    return false;
  }
  *out = static_cast<uint8_t>(parsed - 1);
  return true;
}

}  // namespace util::portal
