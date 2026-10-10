#pragma once
#include <Arduino.h>
// Original short melodies; notes are frequency (Hz) and duration (ms).
namespace Melodies {
struct Note {
  uint16_t hz, ms;
};
const Note morning[] PROGMEM = {{523, 180}, {659, 180}, {784, 360}, {0, 120},
                                {659, 180}, {698, 180}, {880, 360}, {784, 180},
                                {659, 180}, {523, 540}, {0, 700}};
const Note arcade[] PROGMEM = {{659, 130}, {784, 130},  {1047, 260}, {784, 130},
                               {880, 130}, {1175, 260}, {1047, 130}, {880, 130},
                               {784, 260}, {659, 260},  {523, 390},  {0, 700}};
const Note stars[] PROGMEM = {{392, 250}, {523, 250}, {659, 500}, {587, 250},
                              {784, 500}, {659, 250}, {523, 500}, {440, 250},
                              {587, 500}, {523, 750}, {0, 700}};
const Note journey[] PROGMEM = {{523, 160}, {587, 160}, {659, 320}, {784, 320},
                                {659, 160}, {587, 160}, {698, 320}, {880, 320},
                                {784, 160}, {659, 160}, {523, 640}, {0, 700}};
class Player {
  bool playing = false;
  unsigned index = 0, current = 0;
  uint32_t due = 0;

public:
  void update(bool active, unsigned tune, uint32_t now) {
    if (!active) {
      if (playing)
        noTone(12);
      playing = false;
      return;
    }
    if (!playing || current != tune) {
      playing = true;
      current = tune;
      index = 0;
      due = now;
    }
    if (int32_t(now - due) < 0)
      return;
    const Note *notes = tune == 1 ? arcade : tune == 2 ? stars : tune == 3 ? journey : morning;
    unsigned length = tune == 1   ? sizeof(arcade) / sizeof(Note)
                      : tune == 2 ? sizeof(stars) / sizeof(Note)
                      : tune == 3 ? sizeof(journey) / sizeof(Note)
                                  : sizeof(morning) / sizeof(Note);
    Note note;
    memcpy_P(&note, notes + index, sizeof(note));
    if (note.hz)
      tone(12, note.hz, note.ms * 9 / 10);
    else
      noTone(12);
    due = now + note.ms;
    index = (index + 1) % length;
  }
};
} // namespace Melodies
