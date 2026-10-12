#pragma once
#include <Arduino.h>
// Bounded JSON emission into any Print sink. No document or escaped String copy.
class JsonFields {
  Print &out;
  bool first = true;
  void quoted(const String &value) {
    out.write('"');
    for (unsigned i = 0; i < value.length(); ++i) {
      const uint8_t c = value[i];
      if (c == '"' || c == '\\') { out.write('\\'); out.write(c); }
      else if (c < 32 || c == '<') {
        static const char hex[] = "0123456789abcdef";
        out.print("\\u00"); out.write(hex[c >> 4]); out.write(hex[c & 15]);
      } else out.write(c);
    }
    out.write('"');
  }
public:
  explicit JsonFields(Print &sink) : out(sink) { out.write('{'); }
  void key(const char *name) {
    if (!first) out.write(',');
    first = false;
    out.write('"'); out.print(name); out.print("\":");
  }
  void field(const char *name, const String &value) { key(name); quoted(value); }
  void field(const char *name, bool value) { key(name); out.print(value ? "true" : "false"); }
  template<class T> void field(const char *name, T value) { key(name); out.print(value); }
  void finish() { out.write('}'); }
};
