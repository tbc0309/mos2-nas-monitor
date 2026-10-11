#pragma once
#include "sampling_policy.h"

namespace Sampling {
// Each collector owns its next deadline and retry history. One failed group
// cannot postpone another group; deadlines use wrap-safe millisecond arithmetic.
struct OidRetry {
  uint32_t due = 0;
  bool used = false, waiting = false;
  bool timeout(uint32_t now) {
    if (used) return false;
    used = waiting = true; due = now + 500; return true;
  }
  void success() { used = waiting = false; }
  bool ready(uint32_t now) const { return !waiting || int32_t(now - due) >= 0; }
};
struct Schedule {
  uint32_t due = 0;
  unsigned failures = 0;
  bool initialized = false;
  bool ready(uint32_t now) const {
    return !initialized || int32_t(now - due) >= 0;
  }
  void finishCycle(uint32_t now, bool ok, uint32_t period, uint32_t started) {
    if (!ok) { finish(now, false, period); return; }
    uint32_t elapsed = now - started;
    uint32_t remaining = elapsed < period ? period - elapsed : MinimumCycleGap;
    finish(now, true, remaining < MinimumCycleGap ? MinimumCycleGap : remaining);
  }
  void finish(uint32_t now, bool ok, uint32_t period) {
    failures = ok ? 0 : (failures < 4 ? failures + 1 : failures);
    uint32_t wait = period;
    if (!ok) {
      wait = period < RetryFloor ? RetryFloor : period;
      for (unsigned n = 1; n < failures && wait < RetryCeiling; n++)
        wait *= 2;
      if (wait > RetryCeiling) wait = RetryCeiling;
    }
    due = now + wait;
    initialized = true;
  }
};
} // namespace Sampling
