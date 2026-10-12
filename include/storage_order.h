#pragma once
#include <stddef.h>

namespace StorageOrder {
// Natural order without integer conversion: "2" precedes "10", even for long
// SNMP indexes. Sort presentation copies; timestamps stay attached to raw rows.
inline bool less(const char *a, const char *b) {
  while (*a && *b) {
    if (*a >= '0' && *a <= '9' && *b >= '0' && *b <= '9') {
      const char *ae = a, *be = b;
      while (*ae >= '0' && *ae <= '9') ++ae;
      while (*be >= '0' && *be <= '9') ++be;
      while (a + 1 < ae && *a == '0') ++a;
      while (b + 1 < be && *b == '0') ++b;
      if (ae - a != be - b) return ae - a < be - b;
      while (a < ae) { if (*a != *b) return *a < *b; ++a; ++b; }
      a = ae; b = be;
    } else {
      if (*a != *b) return static_cast<unsigned char>(*a) < static_cast<unsigned char>(*b);
      ++a; ++b;
    }
  }
  return !*a && *b;
}

// Ordered read-only view: preserve telemetry row/timestamp positions without
// duplicating every String and capacity value in the physical display state.
template<class Row, unsigned Capacity> class View {
  const Row *rows = nullptr;
  unsigned char order[Capacity] = {};
public:
  template<class Key> void bind(const Row *source, unsigned count, Key key) {
    rows = source;
    if (count > Capacity) count = Capacity;
    for (unsigned i = 0; i < count; ++i) order[i] = i;
    for (unsigned i = 1; i < count; ++i)
      for (unsigned j = i; j && less(key(rows[order[j]]), key(rows[order[j-1]])); --j) {
        const unsigned char value = order[j]; order[j] = order[j-1]; order[j-1] = value;
      }
  }
  const Row &operator[](unsigned index) const { return rows[order[index]]; }
};
template<class Row, class Key> void sort(Row *rows, unsigned count, Key key) {
  for (unsigned i = 1; i < count; ++i)
    for (unsigned j = i; j && less(key(rows[j]), key(rows[j - 1])); --j) {
      Row temporary = rows[j]; rows[j] = rows[j - 1]; rows[j - 1] = temporary;
    }
}
}
