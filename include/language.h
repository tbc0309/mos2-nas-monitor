#pragma once
#include <Arduino.h>
#include "cjk_font.h"
namespace Ui {
bool chinese = true;
}
#include "locales/lcd_zh.h"
namespace Ui {
template <class T>
void draw(T &s, String label, int x, int y, int font, uint16_t color, int datum = TL_DATUM) {
  // Skip translation, width lookup and temporary strings for other display tiles.
  int height = font == 4 ? 26 : font == 3 ? 20 : font == 1 ? 12 : 16;
  int top = datum == MC_DATUM ? y - height / 2 : y;
  if (top >= Cjk::clipBottom || top + height <= Cjk::clipTop)
    return;
  label = tr(label);
  s.setTextColor(color);
  s.setTextDatum(datum);
  int limit = datum == TR_DATUM ? x : datum == MC_DATUM ? 2 * (x < 240 - x ? x : 240 - x) : 240 - x;
  if ((font == 2 || font == 3) && Cjk::width(label, font) > limit)
    font = 1;
  if (font == 1 || font == 2 || font == 3 || font == 4 || Cjk::hasChinese(label)) {
    if (Cjk::width(label, font) > limit) {
      String shortened;
      unsigned pos = 0;
      int used = 0, step = font == 1 ? 12 : 16;
      while (pos < label.length()) {
        unsigned begin = pos, code = Cjk::next(label, pos);
        int advance = Cjk::advance(code, font);
        if (used + advance + step / 2 * 3 > limit)
          break;
        shortened += label.substring(begin, pos);
        used += advance;
      }
      label = shortened + "...";
    }
#ifdef NATIVE_UI
    s.noteText(label, x, y, color, datum);
#endif
    Cjk::draw(s, label, x, y, font, color, datum);
    return;
  }
  s.drawString(label, x, y, font);
}
} // namespace Ui
