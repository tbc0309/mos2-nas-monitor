#pragma once
#include <stdint.h>
namespace Resources {
inline bool expireSample(uint32_t now, uint32_t &updatedAt, uint32_t deadline) {
  if (!updatedAt || uint32_t(now - updatedAt) <= deadline)
    return false;
  updatedAt = 0;
  return true;
}
inline float memory(uint64_t total, uint64_t free, uint64_t buffers, uint64_t cache) {
  if (!total || free > total)
    return -1;
  uint64_t available = free + buffers + cache;
  return 100.0 * double(total - (available > total ? total : available)) / total;
}
struct Cpu {
  uint32_t current[8] = {}, previous[8] = {};
  unsigned mask = 0;
  bool seeded = false;
  void put(unsigned i, uint32_t n) {
    current[i] = n;
    mask |= 1u << i;
  }
  void reset() {
    mask = 0;
    seeded = false;
  }
  float finish() {
    float result = -1;
    if (mask == 255) {
      uint64_t total = 0, idle = 0;
      bool valid = seeded;
      for (unsigned i = 0; i < 8; i++) {
        uint32_t delta = current[i] - previous[i];
        if (delta > 0x7fffffff)
          valid = false;
        total += delta;
        if (i == 3 || i == 4)
          idle += delta;
        previous[i] = current[i];
      }
      if (valid && total)
        result = 100.0 * double(total - idle) / total;
      seeded = true;
    }
    // A partial read must not discard the last complete baseline. The next
    // complete snapshot can calculate an average over the longer elapsed span.
    mask = 0;
    return result;
  }
};
} // namespace Resources
