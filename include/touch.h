#pragma once
#include <stdint.h>
namespace Touch {
enum Action {
  None = 0,
  Tap = 1,
  DoubleTap = 2,
  SwitchMode = 3,
  Unused = 4,
  Management = 5,
  Setup = 6,
  Reset = 8,
  NextMachine = 9
};
class Button {
  bool raw = false, stable = false;
  uint32_t changed = 0, pressed = 0, lastTap = 0;
  unsigned taps = 0;
  bool held = false;

public:
  void clearBurst() {
    taps = 0;
  }
  Action update(bool down, uint32_t now) {
    if (down != raw) {
      raw = down;
      changed = now;
    }
    if (raw != stable && uint32_t(now - changed) >= (stable && uint32_t(now - pressed) >= 500 ? 120u : 30u)) {
      stable = raw;
      if (stable) {
        pressed = changed;
        held = false;
      } else if (!held && uint32_t(changed - pressed) >= 2000) {
        held = true; taps = 0; return NextMachine;
      } else if (!held && uint32_t(now - pressed) >= 40 && uint32_t(now - pressed) < 1500) {
        taps++;
        lastTap = now;
      }
    }
    if (stable && raw && !held && uint32_t(now - pressed) >= 2000) {
      held = true; taps = 0; return NextMachine;
    }
    // Resolve a complete burst only after release, never navigate between clicks.
    if (taps && !raw && !stable && uint32_t(now - lastTap) >= 350) {
      unsigned count = taps;
      taps = 0;
      return count <= 8 && count != 4 && count != 7 ? Action(count) : None;
    }
    return None;
  }
};
// Bounded FIFO prevents later drawing samples from replacing earlier gestures.
class Actions {
  Action items[8] = {};
  unsigned head = 0, tail = 0;
public:
  unsigned dropped = 0;
  bool pending() const { return tail!=head; }
  void push(Action a) {
    if (a == None) return;
    unsigned next = (head + 1) % 8;
    if (next == tail) { ++dropped; return; }
    items[head] = a; head = next;
  }
  Action pop() {
    if (tail == head) return None;
    Action a = items[tail]; tail = (tail + 1) % 8; return a;
  }
};
class ResetConfirmation {
public:
  bool pending = false;
  uint32_t started = 0;
  void begin(uint32_t now) {
    pending = true;
    started = now;
  }
  void cancel() {
    pending = false;
  }
  void update(uint32_t now) {
    if (pending && uint32_t(now - started) >= 10000)
      cancel();
  }
  bool confirm(unsigned clicks, uint32_t now) {
    update(now);
    if (!pending)
      return false;
    cancel();
    return clicks == 3;
  }
};
} // namespace Touch
