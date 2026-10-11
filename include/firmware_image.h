#pragma once
#include <stddef.h>
#include <stdint.h>

namespace FirmwareImage {
constexpr size_t maximumBytes = 0x100000;
// OTA contract: 2 MiB LittleFS layout and the matching font resource checksum.
inline uint8_t contractByte(size_t i, uint32_t fontCrc) {
  const char prefix[] = "MOS2OTA1";
  if (i < 8) return prefix[i];
  const uint32_t value = i < 12 ? 0x200000u : fontCrc;
  return uint8_t(value >> ((i % 4) * 8));
}
struct Validator {
  size_t expected = 0, received = 0, matched = 0;
  uint32_t crc = 0xffffffffu, expectedCrc = 0, fontCrc = 0;
  bool compatible = false, valid = true;
  void begin(size_t size, uint32_t checksum, uint32_t resource) {
    *this = Validator(); expected = size; expectedCrc = checksum; fontCrc = resource;
    valid = size >= 4096 && size <= maximumBytes;
  }
  bool add(const uint8_t *data, size_t size) {
    if (!valid || size > expected - received) return valid = false;
    for (size_t i = 0; i < size; ++i) {
      const uint8_t b = data[i];
      if (received == 0 && b != 0xe9) valid = false;
      crc ^= b;
      for (unsigned bit = 0; bit < 8; ++bit)
        crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
      if (!compatible) {
        matched = b == contractByte(matched, fontCrc) ? matched + 1 : (b == 'M' ? 1 : 0);
        if (matched == 16) compatible = true;
      }
      ++received;
    }
    return valid;
  }
  bool complete() const {
    return valid && received == expected && compatible && (crc ^ 0xffffffffu) == expectedCrc;
  }
};
} // namespace FirmwareImage
