#pragma once

#include <Arduino.h>

#include <math.h>
#include "storage_order.h"

#include "gauge_pixels.h"

#include "language.h"
namespace Dashboard {

enum Color {
  BG,
  PANEL,
  WHITE,
  MUTED,
  CYAN,
  GREEN,
  RED,
  TRACK,
  GRID,
  ORANGE,
  GRAPH,
  SOFTGREEN,
  SOFTRED,
  EDGEORANGE,
  EDGEGREEN,
  EDGETRACK,
  DOTGREEN,
  DOTRED
};

uint16_t palette[18] = {0x222b, 0x1188, 0xffff, 0xae3a, 0x365e, 0x87e8, 0xfacb, 0x1147,
                        0xc679, 0xfd88, 0xef9d, 0xcf15, 0xfdf7, 0x8bc7, 0x4b67, 0x21a9, 0x5d85, 0xa9e7};

// Network colors are semantic and stay fixed across device/card palettes.
// Dots/strokes use roughly 70%-brightness arrow colors; fill peaks match the arrows.
constexpr uint16_t GraphFillGreen = 0x87e8, GraphFillRed = 0xfacb;
inline uint16_t networkColor(Color color) {
  switch (color) {
  case GREEN: return 0x87e8;
  case RED: return 0xfacb;
  case DOTGREEN: return 0x5d85;
  case DOTRED: return 0xa9e7;
  default: return palette[color];
  }
}
bool indexed = false;

inline uint16_t c(Color color) {
  if (indexed) return uint16_t(color);
  return palette[color];
}

struct Disk {
  String index, name, model;
  int status = -1, health = -1;
  float temp = -1;
};

struct Volume {
  String index, name;
  uint64_t units = 0, size = 0, used = 0;
  bool valid = true;
};

struct State {
  String machineNotice, machineLabel;
  bool privacy = false;

#ifdef ESP8266
  StorageOrder::View<Disk, 12> disks;
  StorageOrder::View<Volume, 8> volumes;
#else
  Disk disks[12];
  Volume volumes[8];
#endif
  unsigned diskCount = 0, volumeCount = 0;

  unsigned diskGroup = 0, volumeGroup = 0;
  bool resetPending = false;

  unsigned animationFrame = 0;
  uint32_t cityScrollMs = 0;
  bool showSeconds = true;
  unsigned weekday = 0;
  bool alarmEnabled = false;
  String alarmTime = "07:00";
  int alarmState = 0;
  unsigned alarmRemaining = 0;

  float cpu = -1, memory = -1, storage = -1, temp = -1, rx = -1, tx = -1;

  uint64_t memoryTotal = 0, memoryAvail = 0, volumeTotal = 0, volumeUsed = 0;

  uint32_t uptime = 0, age = 0, interval = 5;

  bool online = false;

  String security;
  String nas, device, model, volume, iface, source = "SNMP";

  bool weatherEnabled = true, weatherOnline = false;

  float weatherTemp = -999, humidity = -1;
  int aqi = -1, weatherCode = -1;

  String weatherCity, weatherCondition, weatherWind, weatherObserved, clock = "--:--:--",
                                                                      date = "Time syncing";

  int rssi = 0;

  float rxHistory[30], txHistory[30];
  unsigned historyCount = 0;
};

// Fixed city viewport, native pixels, no substring truncation or scaled glyphs.
constexpr int CityLeft = 4, CityWidth = 96;
inline int cityOffset(int overflow, uint32_t elapsed) {
  if (overflow <= 0) return 0;
  const uint32_t hold = 1200, travel = uint32_t(overflow) * 50;
  uint32_t phase = elapsed % (2 * (hold + travel));
  if (phase < hold) return 0;
  if (phase < hold + travel) return (phase - hold) / 50;
  if (phase < 2 * hold + travel) return overflow;
  return overflow - int((phase - 2 * hold - travel) / 50);
}
template <class Surface> void weatherCity(Surface &s, const State &d) {
  const String city = d.weatherCity.length() ? d.weatherCity : String("Weather");
  const int font = Ui::chinese && Cjk::hasChinese(city) && Cjk::width(city, 4) <= CityWidth ? 4 : 3;
  const int width = Cjk::width(city, font);
  const int left = width <= CityWidth ? CityLeft + (CityWidth-width)/2
                                      : CityLeft-cityOffset(width-CityWidth,d.cityScrollMs);
  const int oldLeft=Cjk::clipLeft, oldRight=Cjk::clipRight;
  Cjk::clipLeft=CityLeft; Cjk::clipRight=CityLeft+CityWidth;
#ifdef NATIVE_UI
  s.noteText(city,left,31-Cjk::sizeOf(font)/2,c(WHITE),TL_DATUM);
#endif
  Cjk::draw(s,city,left,31-Cjk::sizeOf(font)/2,font,c(WHITE),TL_DATUM);
  Cjk::clipLeft=oldLeft; Cjk::clipRight=oldRight;
}

inline String pct(float n) {
  return n < 0 ? "--" : String(n, 1) + "%";
}

inline String speed(float n) {
  return n < 0 ? "--" : n >= 1048576 ? String(n / 1048576, 1) + "M/s" : String(n / 1024, 1) + "K/s";
}

inline String capacity(uint64_t n) {
  return n >= 1073741824ULL ? String(double(n) / 1073741824.0, 1) + " GiB"
                            : String(double(n) / 1048576.0, 0) + " MiB";
}

template <class Surface> void alarmIcon(Surface &s, int x, int y) {
  s.fillCircle(x, y, 8, c(ORANGE));
  s.fillCircle(x, y, 6, c(BG));
  s.drawLine(x, y, x, y - 5, c(ORANGE));
  s.drawLine(x, y, x + 4, y + 2, c(ORANGE));
  s.drawLine(x - 10, y - 8, x - 5, y - 11, c(ORANGE));
  s.drawLine(x + 5, y - 11, x + 10, y - 8, c(ORANGE));
  s.drawLine(x - 4, y + 7, x - 6, y + 10, c(ORANGE));
  s.drawLine(x + 4, y + 7, x + 6, y + 10, c(ORANGE));
}
template <class T> void text(T &s, String label, int x, int y, int font, Color color = WHITE) {
  Ui::draw(s, label, x, y, font, c(color));
}

template <class T> void right(T &s, String label, int x, int y, int font, Color color = WHITE) {
  Ui::draw(s, label, x, y, font, c(color), TR_DATUM);
  s.setTextDatum(TL_DATUM);
}

// Numeric values retain their semantic accent; units use the shared muted color.
template <class T>
void metricText(T &s, String label, int x, int y, int font, Color value = WHITE,
                bool bold = false) {
  int height = font == 4 ? 26 : font == 3 ? 20 : font == 1 ? 12 : 16;
  if (y >= Cjk::clipBottom || y + height <= Cjk::clipTop)
    return;
  unsigned pos = 0;
  while (pos < label.length()) {
    unsigned start = pos, code = Cjk::next(label, pos);
    bool number = (code >= '0' && code <= '9') || code == '.' || code == '-';
    String glyph = label.substring(start, pos);
    Cjk::draw(s, glyph, x, y, font, c(number ? value : MUTED), TL_DATUM);
    x += Cjk::advance(code, font);
  }
}
template <class T>
void metricRight(T &s, String label, int x, int y, int font, Color value = WHITE) {
  int height = font == 4 ? 26 : font == 3 ? 20 : font == 1 ? 12 : 16;
  if (y >= Cjk::clipBottom || y + height <= Cjk::clipTop)
    return;
  metricText(s, label, x - Cjk::width(label, font), y, font, value);
}

template <class T> void bar(T &s, int x, int y, int w, float value, Color color) {
  if (y >= Cjk::clipBottom || y + 6 <= Cjk::clipTop)
    return;
  s.fillRoundRect(x, y, w, 6, 3, c(TRACK));
  if (value >= 0) {
    int fill = constrain(int(value * w / 100), 0, w);
    if (fill > 0)
      s.fillRoundRect(x, y, fill, 6, 3, c(color));
  }
}

template <class T> void card(T &s, int x, int y, int w, int h) {
  // Establish the surface even for off-screen tiles. Otherwise a skipped card
  // leaves the previous row's foreground/border colors active.
  Cjk::background = c(PANEL);
  if (y >= Cjk::clipBottom || y + h <= Cjk::clipTop)
    return;
  s.fillRoundRect(x, y, w, h, 7, c(TRACK));
  s.fillRoundRect(x + 1, y + 1, w - 2, h - 2, 6, c(PANEL));
}
template <class T> void arrow(T &s, int x, int y, bool up, Color color) {
  const uint16_t fixed = networkColor(color);
  s.fillRect(x + 4, y + (up ? 5 : 0), 4, 9, fixed);
  if (up)
    s.fillTriangle(x + 1, y + 6, x + 10, y + 6, x + 6, y, fixed);
  else
    s.fillTriangle(x + 1, y + 7, x + 10, y + 7, x + 6, y + 13, fixed);
  s.fillRect(x, y + 14, 12, 2, fixed);
  s.fillRect(x, y + 11, 2, 4, fixed);
  s.fillRect(x + 10, y + 11, 2, 4, fixed);
}
template <class T> void largeArrow(T &s, int x, int y, bool up, Color color) {
  const uint16_t fixed = networkColor(color);
  s.fillRect(x + 7, y + (up ? 8 : 0), 7, 16, fixed);
  if (up)
    s.fillTriangle(x, y + 10, x + 21, y + 10, x + 10, y, fixed);
  else
    s.fillTriangle(x, y + 14, x + 21, y + 14, x + 10, y + 24, fixed);
}
// NAS values: primary 20px, detail 16px, auxiliary 12px.
inline String rateLabel(float n) {
  if (n < 0)
    return "--";
  double scaled = n / (n >= 1048576 ? 1048576.0 : 1024.0);
  return String(scaled, scaled >= 99.95 ? 0 : 1) + (n >= 1048576 ? "M/s" : "K/s");
}
inline int rateFont(float) {
  return 3;
}
// Each overview rate has 87 pixels. Keep the native 26px font stable;
// reduce precision only when a label would overflow its half of the screen.
inline String overviewRateLabel(float n) {
  if (n < 0) return "--";
  const double divisor = n >= 1073741824.0f ? 1073741824.0 :
                         n >= 1048576 ? 1048576.0 : 1024.0;
  const char* unit = n >= 1073741824.0f ? "G/s" : n >= 1048576 ? "M/s" : "K/s";
  double value = n / divisor;
  String label = String(value, value >= 99.95 ? 0 : 1) + unit;
  if (Cjk::width(label, 4) > 87) label = String(value, 0) + unit;
  return label;
}
template <class T> void rateText(T &s, const String &label, int x, int y, int font) {
  // Keep numbers white; unit colors distinguish magnitude without changing size.
  Color unit = label.indexOf('G') >= 0 ? ORANGE : label.indexOf('M') >= 0 ? CYAN : MUTED;
  unsigned pos = 0;
  while (pos < label.length()) {
    unsigned start = pos, code = Cjk::next(label, pos);
    bool number = (code >= '0' && code <= '9') || code == '.' || code == '-';
    Cjk::draw(s, label.substring(start, pos), x, y, font, c(number ? WHITE : unit), TL_DATUM);
    x += Cjk::advance(code, font);
  }
}
template <class T> void throughput(T &s, float n, int x, int y, int font = 3) {
  rateText(s, rateLabel(n), x, y, font);
}
inline uint16_t mixColor(uint16_t f, uint16_t b, int a) {
  return ((((f >> 11) * a + (b >> 11) * (255 - a)) / 255) << 11) |
         (((((f >> 5) & 63) * a + ((b >> 5) & 63) * (255 - a)) / 255) << 5) |
         (((f & 31) * a + (b & 31) * (255 - a)) / 255);
}
template <class T> void graph(T &s, const State &d, int x, int y, int w, int h) {
  if (y >= Cjk::clipBottom || y + h <= Cjk::clipTop)
    return;
  s.fillRoundRect(x, y, w, h, 5, c(GRAPH));
  if (d.historyCount == 0) {
    Cjk::background = c(GRAPH);
    text(s, "Waiting for samples", x + 16, y + h / 2 - 8, 2, MUTED);
    Cjk::background = c(BG);
    return;
  }
  if (d.historyCount == 1) {
    // One measured rate is enough to show a dot, but not an invented segment.
    for (unsigned v = 0; v < 2; ++v) {
      float value = v ? d.txHistory[0] : d.rxHistory[0];
      if (value < 0) continue;
      int base = v ? y + h - 6 : y + h / 2 - 4;
      float peak = value > 1024 ? value : 1024;
      int amplitude = v ? h - h / 2 - 10 : h / 2 - 10;
      s.fillCircle(x + w - 6, base - int(value / peak * amplitude), 3,
                   networkColor(v ? DOTRED : DOTGREEN));
    }
    return;
  }
  unsigned count = d.historyCount < 10 ? d.historyCount : 10, start = d.historyCount - count;
  // Separate scales keep asymmetric upload/download trends visible.
  float peak[2] = {1024, 1024};
  for (unsigned k = start; k < d.historyCount; k++) {
    if (d.rxHistory[k] > peak[0])
      peak[0] = d.rxHistory[k];
    if (d.txHistory[k] > peak[1])
      peak[1] = d.txHistory[k];
  }
  const int middle = y + h / 2, span = w - 12;
  // Equal-height bands: receive above, send below, with a gap at the divider.
  const int base[2] = {middle - 4, y + h - 6};
  const int amplitude[2] = {h / 2 - 10, h - h / 2 - 10};
  for (int k = 1; k < 4; k++)
    s.drawFastHLine(x + 5, y + k * h / 4, w - 10, c(GRID));
  const uint16_t fillColors[] = {GraphFillGreen, GraphFillRed};
  // Interpolate the exact integer endpoints, with no row-grid gaps.
  for (int xx = 0; xx <= span; xx++) {
    unsigned k = 0;
    while (k + 1 < count - 1 && xx > int((k + 1) * span / (count - 1))) ++k;
    int xa = k * span / (count - 1), xb = (k + 1) * span / (count - 1);
    for (int v = 0; v < 2; v++) {
      const float *a = v ? d.txHistory : d.rxHistory;
      if (a[start + k] < 0 || a[start + k + 1] < 0) continue;
      int ya = base[v] - int(a[start + k] / peak[v] * amplitude[v]);
      int yb = base[v] - int(a[start + k + 1] / peak[v] * amplitude[v]);
      int curve = ya + (yb - ya) * (xx - xa) / (xb - xa);
      int top = curve < Cjk::clipTop ? curve + (Cjk::clipTop - curve) / 2 * 2 : curve;
      int bottom = base[v] < Cjk::clipBottom - 1 ? base[v] : Cjk::clipBottom - 1;
      for (int yy = top; yy <= bottom; yy += 2) {
        int depth = base[v] - curve;
        // Fill peaks at 50% dot-color opacity; the polyline remains fully opaque.
        int fade = depth ? 255 * (base[v] - yy) / depth : 255;
        int alpha = 18 + 237 * fade * (128 + fade / 2) / 65025;
        uint16_t shade = mixColor(fillColors[v], c(GRAPH), alpha);
        s.fillRect(x + 6 + xx, yy, 1, yy < base[v] ? 2 : 1, shade);
      }
    }
  }
  for (int v = 0; v < 2; v++) {
    const float *values = v ? d.txHistory : d.rxHistory;
    Color color = v ? DOTRED : DOTGREEN;
    // Draw every line first; later segments must not overwrite existing dots.
    for (unsigned k = 1; k < count; k++) {
      if (values[start + k] < 0 || values[start + k - 1] < 0) continue;
      int xp = x + 6 + k * span / (count - 1), yp = base[v] - int(values[start + k] / peak[v] * amplitude[v]);
      int x0 = x + 6 + (k - 1) * span / (count - 1), y0 = base[v] - int(values[start + k - 1] / peak[v] * amplitude[v]);
      s.drawLine(x0, y0, xp, yp, networkColor(color));
    }
    for (unsigned k = 0; k < count; k++) {
      if (values[start + k] < 0) continue;
      int xp = x + 6 + k * span / (count - 1), yp = base[v] - int(values[start + k] / peak[v] * amplitude[v]);
      s.fillCircle(xp, yp, 3, networkColor(color));
    }
  }
}

// Draw the degree ring explicitly: the built-in TFT fonts do not reliably encode 掳.

template <class T>
void temperature(T &s, float value, int x, int y, int font, int datum, Color color = WHITE,
                 unsigned precision = 0, bool mutedUnit = false) {

  String number = value == -999 ? "--" : String(value, precision);
  int nw = Cjk::width(number, font), unit = Cjk::width("C", font), gap = font == 4 ? 9 : 7;

  int left = datum == MC_DATUM   ? x - (nw + gap + unit) / 2
             : datum == TR_DATUM ? x - nw - gap - unit
                                 : x;

  int top = datum == MC_DATUM ? y - (font == 4 ? 13 : font == 3 ? 10 : 8) : y;

  text(s, number, left, top, font, color);
  text(s, "C", left + nw + gap, top, font, mutedUnit ? MUTED : color);

  s.fillCircle(left + nw + gap / 2, top + (font == 4 ? 5 : 3), font == 4 ? 3 : 2,
               c(mutedUnit ? MUTED : color));
  s.fillCircle(left + nw + gap / 2, top + (font == 4 ? 5 : 3), font == 4 ? 1 : 1, Cjk::background);
}

template <class T> void gauge(T &s, float temp, int cx, int cy, int r) {

  float ratio = temp < 0 ? 0 : constrain(temp / 100.0f, 0.0f, 1.0f);
  Color accent = temp >= 75 ? RED : temp >= 60 ? ORANGE : GREEN;

  // Continuous annulus with supersampled edge coverage, stored in flash.

  if (cy - 44 >= Cjk::clipBottom || cy + 44 <= Cjk::clipTop)
    return;
  // Pixels are stored in row order. Locate this tile instead of scanning the full ring.
  unsigned first = 0, end = sizeof(gaugePixels) / sizeof(gaugePixels[0]);
  while (first < end) {
    unsigned middle = (first + end) / 2;
    int row = cy + int((pgm_read_dword(gaugePixels + middle) >> 7) & 127) - 42;
    if (row < Cjk::clipTop)
      first = middle + 1;
    else
      end = middle;
  }
  for (unsigned i = first; i < sizeof(gaugePixels) / sizeof(gaugePixels[0]); i++) {
    uint32_t pixel = pgm_read_dword(gaugePixels + i);
    int py = cy + int((pixel >> 7) & 127) - 42;
    if (py >= Cjk::clipBottom)
      break;
    unsigned angle = (pixel >> 14) & 511, coverage = (pixel >> 23) & 15;

    bool outer = (pixel >> 27) & 1;
    if (!outer && (temp < 0 || angle > ratio * 300))
      continue;
    Color shade = outer ? TRACK : accent;

    if (coverage < 6)
      shade = shade == ORANGE  ? EDGEORANGE
              : shade == GREEN ? EDGEGREEN
              : shade == TRACK ? EDGETRACK
                               : RED;

    if (py >= Cjk::clipTop && py < Cjk::clipBottom)
      s.fillRect(cx + int(pixel & 127) - 42, py, 1, 1, c(shade));
  }

  temperature(s, temp < 0 ? -999 : temp, cx, cy, 4, MC_DATUM, WHITE, 0, true);
}

template <class T> void header(T &s, const State &d, String title) {
  text(s, title, 10, 7, 2);
  s.fillCircle(220, 14, 3, c(d.online ? GREEN : ORANGE));
  s.drawFastHLine(10, 29, 220, c(TRACK));
}

template <class T> void footer(T &s, unsigned page, unsigned count) {
  s.drawFastHLine(10, 213, 220, c(TRACK));
  for (unsigned k = 0; k < count; k++)
    s.fillCircle(120 - int(count - 1) * 11 / 2 + k * 11, 227, k == page ? 3 : 2,
                 c(k == page ? CYAN : TRACK));
  text(s, "TOUCH", 10, 220, 1, MUTED);
  right(s, String(page + 1) + "/" + String(count), 229, 220, 1, MUTED);
}

} // namespace Dashboard

#include "original_assets.h"
#ifdef NATIVE_UI
#include "original_animation.h"
#endif

namespace Dashboard {

template <class T> void render(T &s, const State &d, unsigned page) {
  Cjk::background = c(BG);
  s.fillScreen(c(BG));

  if (d.machineNotice.length() && !d.resetPending) {
    s.fillRoundRect(8, 82, 224, 65, 8, c(PANEL));
    Cjk::background = c(PANEL);
    Ui::draw(s, d.machineLabel, 120, 97, 1, c(CYAN), MC_DATUM);
    Ui::draw(s, d.machineNotice, 120, 125, 2, c(WHITE), MC_DATUM);
    return;
  }
  if (d.resetPending) {

    Ui::draw(s, "RESET DEVICE?", 120, 48, 3, c(RED), MC_DATUM);
    Ui::draw(s, "3 taps to confirm", 120, 96, 2, c(ORANGE), MC_DATUM);
    Ui::draw(s, "Erases settings/photos", 120, 139, 2, c(RED), MC_DATUM);
    Ui::draw(s, "Cancels after 10s", 120, 184, 2, c(MUTED), MC_DATUM);
    return;
  }

  if (page == 6 || page == 5) {

    String time = d.clock.length() == 8 ? d.clock : "--:--:--";

    OriginalAssets::digit(s, 20, 82, time[0], SOFTRED);
    OriginalAssets::digit(s, 60, 82, time[1], SOFTRED);

    OriginalAssets::digit(s, 101, 82, time[3], SOFTGREEN);
    OriginalAssets::digit(s, 141, 82, time[4], SOFTGREEN);

    // Weather keeps the status icon above seconds; the clock pairs it with the alarm time.
    if (d.alarmEnabled && page == 5)
      alarmIcon(s, 202, 96);
    if (d.showSeconds) {
      OriginalAssets::second(s, 182, 112, time[6], WHITE);
      OriginalAssets::second(s, 202, 112, time[7], WHITE);
    }

    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    String month = d.date.length() == 10 ? String(int(d.date.substring(5, 7).toInt())) : "--",
           day = d.date.length() == 10 ? String(int(d.date.substring(8, 10).toInt())) : "--";

    Ui::draw(s, month + (Ui::chinese ? "月" : "/") + day + (Ui::chinese ? "日" : ""), 54, 166, 3,
             c(WHITE), MC_DATUM);
    Ui::draw(s, days[d.weekday % 7], 131, 166, 3, c(WHITE), MC_DATUM);

#ifdef NATIVE_UI
    if (page == 5)
      OriginalAnimation::draw(s, d.animationFrame);
#endif
    if (page == 6) {

      if (d.alarmEnabled) {
        alarmIcon(s, 120 - Cjk::width(d.alarmTime, 3) / 2 - 16, 198);
        Ui::draw(s, d.alarmTime, 120, 198, 3, c(WHITE), MC_DATUM);
      }

      if (d.alarmState == 1)
        Ui::draw(s, "RINGING", 120, 224, 3, c(ORANGE), MC_DATUM);

      else if (d.alarmState == 2)
        Ui::draw(s, "Snooze " + String(d.alarmRemaining / 60) + "m", 120, 224, 3, c(MUTED),
                 MC_DATUM);

      return;
    }

    int code = d.weatherCode;
    if (code == 8 || code == 21 || code == 22 || code == 301)
      code = 7;
    else if (code == 10 || code == 23 || code == 24)
      code = 9;
    else if (code == 12 || code == 25)
      code = 11;
    else if (code == 26)
      code = 14;
    else if (code == 27)
      code = 15;
    else if (code == 17 || code == 28 || code == 302)
      code = 16;
    else if (code == 32 || code == 49 || code >= 54)
      code = 53;

    OriginalAssets::weather(s, code);
    weatherCity(s, d);

    Color air = d.aqi < 0 ? TRACK : d.aqi > 150 ? RED : d.aqi > 50 ? ORANGE : GREEN;
    s.fillRoundRect(104, 18, 50, 24, 4, c(air));

    Cjk::background = c(air);
    Ui::draw(s,
             d.aqi < 0 ? String("--") : Ui::chinese ? (d.aqi > 300 ? "严重"
                            : d.aqi > 200 ? "重度" : d.aqi > 150 ? "中度"
                            : d.aqi > 100 ? "轻度" : d.aqi > 50 ? "良" : "优")
                         : "AQI " + String(d.aqi),
             129, 31, 3, c(BG), MC_DATUM);
    Cjk::background = c(BG);

    const int weatherFont = (Cjk::width(d.weatherCondition, 4) <= 164 && Cjk::width(d.weatherWind, 4) <= 164) ? 4 : 3;
    String weatherLine = (time[7] - '0') % 6 < 3 ? d.weatherCondition : d.weatherWind;
    Ui::draw(s, weatherLine, 84, 61, weatherFont, c(WHITE), MC_DATUM);

    temperature(s, d.weatherTemp, 128, 197, 3, MC_DATUM, WHITE, 0);
    Ui::draw(s, d.humidity >= 0 ? String(d.humidity, 0) + "%" : "--", 128, 227, 3, c(WHITE),
             MC_DATUM);

    s.fillCircle(27, 200, 7, c(RED));
    s.fillRoundRect(24, 184, 6, 17, 3, c(RED));
    s.fillCircle(27, 228, 7, c(CYAN));
    s.fillTriangle(20, 224, 34, 224, 27, 213, c(CYAN));

    s.fillRoundRect(45, 192, 52, 6, 3, c(WHITE));
    s.fillRoundRect(46, 193, 50, 4, 3, c(BG));
    s.fillRoundRect(46, 193, constrain(int(d.weatherTemp + 10), 0, 50), 4, 3, c(ORANGE));

    s.fillRoundRect(45, 222, 52, 6, 3, c(WHITE));
    s.fillRoundRect(46, 223, 50, 4, 3, c(BG));
    s.fillRoundRect(46, 223, constrain(int(d.humidity / 2), 0, 50), 4, 3, c(CYAN));

    return;
  }

  if (page == 7) {
    text(s, "PHOTO ALBUM", 10, 10, 2);
    text(s, "Upload photos in management", 10, 90, 2, CYAN);
    text(s, "Crop / zoom / rotate", 10, 119, 2, MUTED);
    footer(s, d.weatherEnabled ? 2 : 1, d.weatherEnabled ? 3 : 2);
    return;
  }

  if (page == 8) {
    header(s, d, "DISKS / HEALTH");

    unsigned start = d.diskCount > 4 ? (d.diskGroup % ((d.diskCount + 3) / 4)) * 4 : 0;

    for (unsigned i = start; i < d.diskCount && i < start + 4; i++) {
      const Disk &disk = d.disks[i];
      int y = 39 + (i - start) * 42;
      if (y - 3 >= Cjk::clipBottom || y + 36 <= Cjk::clipTop)
        continue;
      card(s, 6, y - 3, 228, 39);
      Cjk::background = c(PANEL);

      text(s, disk.name, 10, y, 2, CYAN);
      temperature(s, disk.temp < 0 ? -999 : disk.temp, 230, y, 2, TR_DATUM,
                  disk.temp >= 55 ? ORANGE : GREEN, 0, true);

      text(s, disk.model.substring(0, 24), 10, y + 18, 1, MUTED);

      right(s,
            disk.status == 1 && disk.health == 1 ? "OK"
            : disk.status < 0                    ? "--"
                                                 : "CHECK",
            230, y + 18, 1, disk.status == 1 && disk.health == 1 ? GREEN : ORANGE);
    }
    Cjk::background = c(BG);
    if (!d.diskCount)
      text(s, d.model == "FNOS" || d.model == "OpenWrt" ? "No SMART data" : "Discovering disks...", 10, 70, 2, MUTED);
    text(s, String(d.diskCount) + Ui::tr(" drives"), 10, 201, 1, MUTED);
    if (d.diskCount > 4)
      right(s, "Double tap: more", 230, 201, 1, MUTED);
    footer(s, 3, 6);
    return;
  }

  if (page == 9) {
    header(s, d, "ALL VOLUMES");
    unsigned start = d.volumeCount > 3 ? (d.volumeGroup % ((d.volumeCount + 2) / 3)) * 3 : 0;

    for (unsigned i = start; i < d.volumeCount && i < start + 3; i++) {
      const Volume &v = d.volumes[i];
      int y = 39 + (i - start) * 55;
      if (y - 3 >= Cjk::clipBottom || y + 49 <= Cjk::clipTop)
        continue;
      card(s, 6, y - 3, 228, 52);
      Cjk::background = c(PANEL);
      float used = v.valid && v.size ? 100.0 * v.used / v.size : -1;

      String volumeLabel = v.name;
      if (v.name.startsWith("/volumeUSB")) volumeLabel = "USB " + v.name.substring(10, v.name.length());
      if (volumeLabel.length() > 20) volumeLabel = volumeLabel.substring(0, 17) + "...";
      text(s, volumeLabel, 10, y, 2, CYAN);
      metricRight(s, pct(used), 230, y, 2);
      bar(s, 10, y + 23, 220, used, used >= 90 ? RED : GREEN);
      metricText(s, (v.valid ? capacity(v.used * v.units) : String("--")) + " / " +
                       (v.size ? capacity(v.size * v.units) : String("--")), 10, y + 34, 1);
    }
    Cjk::background = c(BG);
    if (!d.volumeCount)
      text(s, "Discovering volumes...", 10, 70, 2, MUTED);
    text(s, String(d.volumeCount) + Ui::tr(" volumes"), 10, 201, 1, MUTED);
    if (d.volumeCount > 3)
      right(s, "Double tap: more", 230, 201, 1, MUTED);
    footer(s, 4, 6);
    return;
  }

  if (page == 0) {

    largeArrow(s, 8, 6, false, GREEN);
    if (Cjk::clipBottom > 6 && Cjk::clipTop < 32) rateText(s, overviewRateLabel(d.rx), 33, 6, 4);
    largeArrow(s, 128, 6, true, RED);
    if (Cjk::clipBottom > 6 && Cjk::clipTop < 32) rateText(s, overviewRateLabel(d.tx), 153, 6, 4);

    graph(s, d, 8, 38, 224, 74);

    card(s, 6, 120, 131, 78);
    s.fillCircle(186, 165, 44, c(TRACK));
    s.fillCircle(186, 165, 43, c(PANEL));
    Cjk::background = c(PANEL);
    text(s, "CPU", 10, 126, 2);
    if (Cjk::clipBottom > 124 && Cjk::clipTop < 150) metricRight(s, pct(d.cpu), 133, 124, 4);
    bar(s, 10, 151, 123, d.cpu, CYAN);

    text(s, "Memory", 10, 166, 2);
    if (Cjk::clipBottom > 164 && Cjk::clipTop < 190) metricRight(s, d.memory < 0 ? "--" : String(d.memory, 0) + "%", 133, 164, 4);
    bar(s, 10, 191, 123, d.memory, CYAN);

    gauge(s, d.temp, 186, 165, 41);

    Cjk::background = c(BG);
    text(s, d.nas, 10, 198, 2, MUTED);
    if(d.privacy){
      int x=128; // Align the lock right edge with the memory card.
      s.fillRoundRect(x,204,9,7,2,c(GREEN));
      s.fillRoundRect(x+2,198,5,9,2,c(GREEN));
      s.fillRoundRect(x+3,200,3,5,1,c(BG));
      s.fillRect(x+4,206,1,3,c(BG));
    }
    s.fillCircle(143, 205, 2, c(d.online ? GREEN : ORANGE));

  } else if (page == 1) {
    header(s, d, "NETWORK");
    card(s, 6, 37, 228, 69);
    Cjk::background = c(PANEL);
    largeArrow(s, 12, 41, false, GREEN);
    text(s, "RECEIVE", 40, 45, Ui::chinese ? 2 : 1, MUTED);
    if (Cjk::clipBottom > 41 && Cjk::clipTop < 67) throughput(s, d.rx, 228 - Cjk::width(rateLabel(d.rx), 4), 41, 4);
    largeArrow(s, 12, 76, true, RED);
    text(s, "SEND", 40, 80, Ui::chinese ? 2 : 1, MUTED);
    if (Cjk::clipBottom > 76 && Cjk::clipTop < 102) throughput(s, d.tx, 228 - Cjk::width(rateLabel(d.tx), 4), 76, 4);
    Cjk::background = c(BG);
    graph(s, d, 8, 112, 224, 70);
    text(s, "IF: " + d.iface, 10, 192, 2, MUTED);

  } else if (page == 2) {
    header(s, d, "CPU / MEMORY");
    card(s, 6, 35, 228, 47);
    card(s, 6, 85, 228, 47);
    card(s, 6, 136, 228, 49);
    Cjk::background = c(PANEL);
    text(s, "CPU usage", 10, 43, 2, MUTED);
    if (Cjk::clipBottom > 40 && Cjk::clipTop < 66) metricRight(s, pct(d.cpu), 230, 40, 4);
    bar(s, 10, 72, 220, d.cpu, CYAN);
    text(s, "Memory usage", 10, 93, 2, MUTED);
    if (Cjk::clipBottom > 90 && Cjk::clipTop < 116) metricRight(s, pct(d.memory), 230, 90, 4);
    bar(s, 10, 122, 220, d.memory, CYAN);
    text(s, "Used", 10, 143, 2, MUTED);
    if (Cjk::clipBottom > 141 && Cjk::clipTop < 161) metricRight(s, d.memoryTotal && d.memory >= 0 ? capacity(d.memoryTotal - d.memoryAvail) : "--", 230, 141, 3);
    text(s, "Total", 10, 167, 2, MUTED);
    if (Cjk::clipBottom > 165 && Cjk::clipTop < 185) metricRight(s, d.memoryTotal ? capacity(d.memoryTotal) : "--", 230, 165, 3);
    Cjk::background = c(BG);
    text(s, "RAM excludes buffers/cache", 10, 195, 1, MUTED);

  } else if (page == 4) {
    header(s, d, "DEVICE / STATUS");
    card(s, 6, 36, 228, 130);
    Cjk::background = c(PANEL);
    if (Cjk::clipBottom > 42 && Cjk::clipTop < 62)
      text(s, d.model.length() ? d.model : "SNMP", 10, 42, Cjk::width(d.model,3)>220?2:3, WHITE);
    // Layout is language-dependent, not data-dependent; calculate it once.
    static int valueColumns[2] = {0,0};
    int &valueX = valueColumns[Ui::chinese ? 1 : 0];
    const char* labels[] = {"Device", "Up", "MOS2", "WiFi"};
    const Color rowColors[] = {CYAN, WHITE, CYAN, MUTED};
    if (!valueX) {
      valueX = 10;
      for (const auto label : labels) {
        int edge = 18 + Cjk::width(Ui::tr(label), 2);
        if (edge > valueX) valueX = edge;
      }
    }
    for (unsigned row = 0; row < 4; ++row) {
      const int y = 73 + 23 * row;
      if (y >= Cjk::clipBottom || y + 16 <= Cjk::clipTop) continue;
      String value;
      if (row == 0) value = d.nas;
      else if (row == 1) value = !d.uptime ? Ui::tr("WAIT") : d.uptime<3600 ?
        String(d.uptime/60)+Ui::tr("min ")+String(d.uptime%60)+Ui::tr("sec") :
        String(d.uptime/86400)+Ui::tr("day ")+String((d.uptime/3600)%24)+Ui::tr("hr");
      else if (row == 2) value = d.device;
      else value = String(d.rssi) + " dBm";
      text(s, labels[row], 10, y, 2, rowColors[row]);
      text(s, value, valueX, y, 2, rowColors[row]);
    }
    card(s, 6, 171, 228, 26);
    Cjk::background = c(PANEL);
    if (Cjk::clipBottom > 176 && Cjk::clipTop < 192) {
      String snmpStatus = d.source + " " + (d.online ? Ui::tr("OK") +
        (d.security.length() ? " " + d.security : String("")) :
        Ui::tr("No response") + (d.age ? " " + String(d.age) + Ui::tr("s") : String("")));
      text(s, snmpStatus, 10, 176, 2, d.online ? GREEN : ORANGE);
    }
    Cjk::background = c(BG);
    text(s, "5 taps: management QR", 10, 200, 1, MUTED);
  }

  footer(s, page == 4 ? 5 : page, 6);
}

} // namespace Dashboard
