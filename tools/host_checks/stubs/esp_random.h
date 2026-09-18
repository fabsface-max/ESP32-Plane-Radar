#pragma once
#include <cstdint>
/** Host stub: a fixed value is enough, the test never checks randomness. */
inline uint32_t esp_random() { return 0x1a2b3c4du; }
