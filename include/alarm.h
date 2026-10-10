#pragma once
#include <stdint.h>
namespace Alarm {
enum State { Idle, Ringing, Snoozed, Dismissed };
class Clock {
public:
  State state = Idle;
  uint32_t due = 0, lastDay = 0, ringStarted = 0;
  unsigned taps = 0;
  uint32_t lastTap = 0;
  bool active() const {
    return state == Ringing || state == Snoozed;
  }
  bool update(bool enabled, unsigned minute, unsigned target, uint32_t day, uint32_t epoch,
              unsigned snooze, bool allowedDay = true) {
    if (!enabled) {
      state = Idle;
      return false;
    }
    if (epoch < 1700000000)
      return false;
    if (allowedDay && minute == target && day != lastDay) {
      lastDay = day;
      state = Ringing;
      ringStarted = epoch;
      taps = 0;
      return true;
    }
    if (state == Snoozed && epoch >= due) {
      state = Ringing;
      ringStarted = epoch;
      taps = 0;
    }
    if (state == Ringing && epoch - ringStarted >= 60) {
      state = Snoozed;
      due = epoch + snooze * 60;
      taps = 0;
    }
    return false;
  }
  void tap(uint32_t now, uint32_t epoch, unsigned snooze) {
    if (!active())
      return;
    taps = taps && uint32_t(now - lastTap) <= 800 ? taps + 1 : 1;
    lastTap = now;
    if (taps >= 3) {
      state = Dismissed;
      taps = 0;
      due = 0;
      return;
    }
    if (state == Ringing) {
      state = Snoozed;
      due = epoch + snooze * 60;
    }
  }
  void dismiss() {
    state = Dismissed;
    due = 0;
    taps = 0;
  }
};
} // namespace Alarm
