#pragma once
// GPIO interrupts record edges only. Gesture recognition and all side effects
// remain in the main loop, even while HTTP or drawing temporarily blocks it.
namespace TouchInput {
constexpr uint8_t capacity = 32;
volatile uint8_t head = 0, tail = 0;
volatile bool overflow = false;
volatile uint32_t edges = 0;
uint32_t overflows = 0, gestures = 0;
Touch::Actions actions;
volatile uint32_t times[capacity];
volatile bool levels[capacity];
}
void IRAM_ATTR recordTouchEdge() {
  using namespace TouchInput;
  ++edges;
  const uint8_t next = (head + 1) % capacity;
  if (next == tail) { overflow = true; return; }
  times[head] = millis();
  levels[head] = !digitalRead(4);
  head = next;
}
void pollTouchInput() {
  using namespace TouchInput;
  // A noisy electrode must not refill the interrupt queue faster than this
  // loop drains it and starve Wi-Fi/watchdog service. Finish at most one queue
  // per visit; unprocessed edges retain their timestamps for the next visit.
  for (unsigned budget = capacity; budget; --budget) {
    noInterrupts();
    if (overflow) {
      tail = head; overflow = false; ++overflows;
      touchButton = Touch::Button();
    }
    if (tail == head) { interrupts(); break; }
    const uint32_t at = times[tail];
    const bool down = levels[tail];
    tail = (tail + 1) % capacity;
    interrupts();
    // Confirm the previous level before applying its next edge. This preserves
    // the actual press duration when the loop did not run between two edges.
    Touch::Action pending = touchButton.update(!down, at);
    if (pending != Touch::None) { actions.push(pending); ++gestures; }
    pending = touchButton.update(down, at);
    if (pending != Touch::None) { actions.push(pending); ++gestures; }
  }
  // Do not advance gesture time past edges still waiting in the queue.
  if (tail != head) return;
  Touch::Action pending = touchButton.update(!digitalRead(4), millis());
  if (pending != Touch::None) { actions.push(pending); ++gestures; }
}

Touch::Action sampleTouch() {
  pollTouchInput();
  return TouchInput::actions.pop();
}
