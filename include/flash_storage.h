#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace Storage {
inline bool isSystemResource(const char* path) {
  if (*path == '/') path++;
  return strncmp(path, "fonts/", 6) == 0 || strncmp(path, "assets/", 7) == 0;
}
// Never format a mount failure unless every word in the partition is erased.
template <class Read, class Service>
bool isErased(uint32_t begin, uint32_t end, Read read, Service service) {
  if (begin >= end || (begin & 3) || (end & 3))
    return false;
  uint32_t words[256];
  for (uint32_t address = begin; address < end;) {
    size_t bytes = end - address < sizeof(words) ? end - address : sizeof(words);
    if (!read(address, words, bytes))
      return false;
    for (size_t i = 0; i < bytes / sizeof(uint32_t); i++)
      if (words[i] != 0xffffffffu)
        return false;
    address += bytes;
    service();
  }
  return true;
}
} // namespace Storage
