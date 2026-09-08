#pragma once
// Host stub. Just enough Arduino for the portal page code to compile and run
// off the board; the firmware build never sees this file.
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string>

class String {
 public:
  String() = default;
  String(const char* s) : value_(s != nullptr ? s : "") {}
  const char* c_str() const { return value_.c_str(); }
  bool operator==(const char* other) const { return value_ == other; }
  bool operator!=(const char* other) const { return !(*this == other); }

 private:
  std::string value_;
};

struct HostSerial {
  void println(const char* text) { (void)text; }
  void printf(const char* format, ...) { (void)format; }
};
extern HostSerial Serial;
