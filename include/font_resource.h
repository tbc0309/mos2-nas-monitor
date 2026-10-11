#pragma once
#include <stdint.h>
#include <string.h>
#include "font_index.h"
#ifdef ESP8266
#include <LittleFS.h>
#else
#include <fstream>
#include <cstdlib>
#include <string>
#endif

// The resource pack is independent of user configuration and is retained by factory reset.
namespace FontResource {
class Source {
#ifdef ESP8266
  File file;
#else
  std::ifstream file;
#endif
public:
  bool open() {
#ifdef ESP8266
    file = LittleFS.open("/fonts/ui04.bin", "r");
    return bool(file);
#else
    const char* overridePath = std::getenv("MOS2_FONT_RESOURCE");
    std::string path = overridePath ? overridePath : "data/fonts/ui04.bin";
    file.open(path, std::ios::binary);
    if (!file && !overridePath) {
      file.clear();
      std::string here = __FILE__;
      path = here.substr(0, here.find_last_of("/\\")) + "/../data/fonts/ui04.bin";
      file.open(path, std::ios::binary);
    }
    return bool(file);
#endif
  }
  void close() {
#ifdef ESP8266
    file.close();
#else
    if (file.is_open()) file.close();
    file.clear();
#endif
  }
  uint32_t size() {
#ifdef ESP8266
    return file.size();
#else
    file.seekg(0, std::ios::end); return uint32_t(file.tellg());
#endif
  }
  bool read(uint32_t offset, uint8_t* bytes, size_t count) {
#ifdef ESP8266
    return file.seek(offset, SeekSet) && file.read(bytes, count) == count;
#else
    file.clear(); file.seekg(offset); file.read(reinterpret_cast<char*>(bytes), count);
    return size_t(file.gcount()) == count;
#endif
  }
};
static Source source;
static bool attempted = false, ready = false;
static uint32_t reads = 0, cacheHits = 0, readErrors = 0, tick = 0;
struct Entry {
  uint16_t index = 0;
  uint8_t font = 0;
  bool valid = false;
  uint32_t stamp = 0;
  uint8_t pixels[338] = {};
};
// Twelve packed glyphs use about 4 KiB, rather than loading the whole font into RAM.
static Entry cache[12];
static int lastEntry = -1;
inline uint32_t word(const uint8_t* p) {
  return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
inline bool begin() {
  source.close(); attempted = true; ready = false; lastEntry = -1;
  for (auto& e : cache) e.valid = false;
  if (!source.open() || source.size() != Cjk::resourceBytes) return false;
  uint8_t header[24];
  if (!source.read(0, header, sizeof(header)) || memcmp(header, "MOSFONT2", 8) ||
      word(header + 8) != Cjk::resourceBytes || word(header + 12) != Cjk::resourceCrc ||
      word(header + 16) != 2 || word(header + 20) != 16) return false;
  uint8_t buffer[512]; uint32_t crc = 0xffffffff;
  for (uint32_t p = 24; p < Cjk::resourceBytes;) {
    size_t n = Cjk::resourceBytes - p;
    if (n > sizeof(buffer)) n = sizeof(buffer);
    if (!source.read(p, buffer, n)) return false;
    for (size_t i = 0; i < n; i++) {
      crc ^= buffer[i];
      for (int bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
    p += n;
#ifdef ESP8266
    yield();
#endif
  }
  ready = (crc ^ 0xffffffffu) == Cjk::resourceCrc;
  return ready;
}
inline bool available() { return attempted ? ready : begin(); }
inline void put(Entry& e, unsigned position, uint8_t alpha) {
  e.pixels[position / 2] |= alpha << ((position % 2) * 4);
}
inline bool decode(Entry& entry, int i, int font) {
  int size = font == 4 ? 26 : font == 3 ? 20 : font == 1 ? 12 : 16;
  // Small banks index the full common alphabet; large banks index all UI glyphs.
  int g = i;
  memset(entry.pixels, 0, sizeof(entry.pixels));
  uint8_t input[1024];
  const uint32_t* offsets = font == 1 ? Cjk::offsets12 : font == 2 ? Cjk::offsets16 :
      font == 3 ? Cjk::offsets20 : Cjk::offsets26;
  uint32_t bank = font == 1 ? Cjk::resourceData12 : font == 2 ? Cjk::resourceData16 :
      font == 3 ? Cjk::resourceData20 : Cjk::resourceData26;
  uint32_t first = pgm_read_dword(offsets + g), end = pgm_read_dword(offsets + g + 1);
  if (end < first || end - first > sizeof(input) || bank + end > Cjk::resourceBytes ||
      !source.read(bank + first, input, end - first)) return false;
  unsigned k = 0, pos = 0, bytes = end - first;
  while (k < bytes) {
    uint8_t token = input[k++]; unsigned n = (token & 127) + 1;
    if (pos + n > unsigned(size*size)) return false;
    if (token & 128) {
      if (k + (n+1)/2 > bytes) return false;
      for (unsigned j = 0; j < n; j++) put(entry, pos++, (input[k+j/2] >> ((j%2)*4)) & 15);
      k += (n+1)/2;
    } else pos += n;
  }
  return pos == unsigned(size*size);
}
inline int coverage(int i, int x, int y, int font) {
  if (i < 0 || !available()) return 0;
  int size = font == 4 ? 26 : font == 3 ? 20 : font == 1 ? 12 : 16;
  if (x < 0 || y < 0 || x >= size || y >= size) return 0;
  int selected = lastEntry;
  if (selected < 0 || !cache[selected].valid || cache[selected].index != i || cache[selected].font != font) {
    selected = -1;
    for (int j = 0; j < 12; j++) if (cache[j].valid && cache[j].index == i && cache[j].font == font) { selected = j; break; }
    if (selected >= 0) cacheHits++;
    else {
      selected = 0;
      for (int j = 0; j < 12; j++) {
        if (!cache[j].valid) { selected = j; break; }
        if (cache[j].stamp < cache[selected].stamp) selected = j;
      }
      Entry& e = cache[selected]; e.valid = false;
      if (!decode(e, i, font)) { readErrors++; return 0; }
      reads++; e.index = i; e.font = font; e.valid = true;
    }
    lastEntry = selected;
    cache[selected].stamp = ++tick;
  }
  unsigned p = y * size + x;
  return (cache[selected].pixels[p/2] >> ((p%2)*4)) & 15;
}
}
