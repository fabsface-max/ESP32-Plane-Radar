#include "util/callsign.h"

namespace util::callsign {

namespace {

bool isAlpha(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool isDigit(char c) { return c >= '0' && c <= '9'; }

char toUpper(char c) { return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 32) : c; }

}  // namespace

bool icaoPrefix(const char* text, char out[4]) {
  if (text == nullptr || out == nullptr) {
    return false;
  }
  for (int i = 0; i < 3; ++i) {
    if (!isAlpha(text[i])) {
      return false;
    }
  }
  const char fourth = text[3];
  if (!isDigit(fourth) && !isAlpha(fourth)) {
    return false;
  }
  // A registration like "DEABC" has no digit at all; an airline flight number
  // always carries one.
  bool has_digit = false;
  for (const char* p = text + 3; *p != '\0'; ++p) {
    if (isDigit(*p)) {
      has_digit = true;
      break;
    }
  }
  if (!has_digit) {
    return false;
  }

  for (int i = 0; i < 3; ++i) {
    out[i] = toUpper(text[i]);
  }
  out[3] = '\0';
  return true;
}

}  // namespace util::callsign
