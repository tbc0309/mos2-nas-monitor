#pragma once
#include <stdint.h>

namespace MonitorColors {
constexpr uint16_t rgb(unsigned r, unsigned g, unsigned b) {
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}
// Use the actual RGB565 surface, not a preset name. This also supports custom
// light backgrounds with dark cards, without allocating another theme buffer.
inline bool light(uint16_t surface) {
  const unsigned r = ((surface >> 11) & 31) * 255 / 31;
  const unsigned g = ((surface >> 5) & 63) * 255 / 63;
  const unsigned b = (surface & 31) * 255 / 31;
  return 299 * r + 587 * g + 114 * b >= 148000;
}
}
