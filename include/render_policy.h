#pragma once
#include <stdint.h>

namespace RenderPolicy {
// Select the largest useful RGB565 strip while preserving the Wi-Fi/TCP reserve.
// A one-row strip is a last resort, not a fixed low-memory threshold.
inline unsigned tileHeight(uint32_t heap,uint32_t block,unsigned width=240) {
  if(!width || width>240) return 0;
  constexpr uint32_t reserve=7000, allocatorMargin=256;
  const uint32_t rowBytes=width*2;
  if (heap<reserve+rowBytes || block<rowBytes+allocatorMargin) return 0;
  unsigned rows=(heap-reserve)/rowBytes;
  unsigned contiguous=(block-allocatorMargin)/rowBytes;
  if(rows>contiguous) rows=contiguous;
  if(rows>15) rows=15;
  return rows;
}
} // namespace RenderPolicy
