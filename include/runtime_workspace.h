#pragma once
// Main-loop jobs run sequentially. HTTP Flash transfer, JSON chunks and config
// encryption reuse one bounded workspace instead of competing heap allocations.
namespace RuntimeWorkspace {
alignas(8) static uint8_t bytes[1024];
}
