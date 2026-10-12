#pragma once
#include <stdint.h>
namespace Alarm {
enum State { Idle, Ringing, Snoozed, Dismissed };
class Clock {
public:
  State state = Idle;
  uint32_t due = 0, lastDay = 0, ringStarted = 0;
  unsigned taps = 0;
  bool snoozeUsed = false;
  // Each ringing period is bounded; a day permits one user-requested snooze.
  void start(uint32_t epoch) {
    state = Ringing; ringStarted = epoch; taps = 0; due = 0; snoozeUsed = false;
  }
  uint32_t lastTap = 0;
  bool active() const {
    return state == Ringing || state == Snoozed;
  }
  bool update(bool enabled, unsigned minute, unsigned target, uint32_t day, uint32_t epoch,
              unsigned /*snooze*/, bool allowedDay = true) {
    if (!enabled) {
      state = Idle;
      return false;
    }
    // Invalid network time must not prevent an already started alarm stopping.
    if (epoch >= 1700000000 && allowedDay && minute == target && day != lastDay) {
      lastDay = day;
      start(epoch);
      return true;
    }
    if (state == Snoozed && epoch >= due) {
      state = Ringing;
      ringStarted = epoch;
      taps = 0;
    }
    if (state == Ringing && epoch - ringStarted >= 300) {
      dismiss();
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
      if (snoozeUsed) { dismiss(); return; }
      snoozeUsed = true;
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
