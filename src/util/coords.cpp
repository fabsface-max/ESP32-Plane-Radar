#include "util/coords.h"

#include <cmath>
#include <cstdlib>

namespace util::coords {

bool parseCoordinate(const char* text, double* out) {
  if (text == nullptr || out == nullptr || text[0] == '\0') {
    return false;
  }

  char* end = nullptr;
  const double value = std::strtod(text, &end);
  if (end == text || end == nullptr || *end != '\0') {
    return false;
  }
  if (!std::isfinite(value)) {
    return false;
  }

  *out = value;
  return true;
}

}  // namespace util::coords
