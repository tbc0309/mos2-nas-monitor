#pragma once
#include <stdint.h>
namespace Sampling {
// Healthy live metrics release the first storage scan. A missing OID must
// not permanently hide disks; explicit storage pages also take priority.
inline bool storageBootstrapReady(uint32_t elapsed, unsigned liveMask, unsigned page) {
  return (liveMask & 15u) == 15u || page == 8 || page == 9 || elapsed >= 120000u;
}
// Group order matches the six live collectors, then disk, volume and interface.
inline uint32_t startupDelay(unsigned page, unsigned group) {
  static const uint8_t desktop[] = {20,21,23,22,25,30,35,40,22};
  static const uint8_t nas[] = {30,31,32,33,45,60,65,70,30};
  if (page >= 5 && page <= 7) return desktop[group] * 1000u;
  if (page == 0 && group < 4) return 0;
  if (page == 1 && (group == 2 || group == 8)) return 0;
  if (page == 2 && group < 2) return 0;
  if (page == 4 && (group == 4 || group == 5 || group == 8)) return 0;
  if (page == 8 && group == 6) return 0;
  if (page == 9 && group == 7) return 0;
  if (page == 0 && group == 8) return 0;
  return nas[group] * 1000u;
}
struct StartupPlan {
  uint32_t due[9] = {};
  unsigned lastPage = 0;
  bool initialized = false;
  void reset(uint32_t now, unsigned page) {
    lastPage = page; initialized = true;
    for (unsigned i = 0; i < 9; ++i) due[i] = now + startupDelay(page, i);
  }
  void promote(uint32_t now, unsigned page) {
    if (!initialized) { reset(now, page); return; }
    if (page == lastPage) return;
    lastPage = page;
    for (unsigned i = 0; i < 9; ++i) {
      uint32_t earlier = now + startupDelay(page, i);
      if (int32_t(earlier - due[i]) < 0) due[i] = earlier;
    }
  }
  bool ready(uint32_t now, unsigned group) const {
    return initialized && int32_t(now - due[group]) >= 0;
  }
};
}
