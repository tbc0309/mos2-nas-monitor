#include <cassert>
#include <cstring>
#include <vector>
#include "flash_storage.h"
int main() {
  std::vector<uint32_t> memory(777, 0xffffffffu);
  unsigned services = 0;
  auto read = [&](uint32_t address, uint32_t *out, size_t bytes) {
    if (address + bytes > memory.size() * 4)
      return false;
    std::memcpy(out, reinterpret_cast<unsigned char *>(memory.data()) + address, bytes);
    return true;
  };
  auto service = [&]() { services++; };
  assert(Storage::isErased(0, memory.size() * 4, read, service));
  assert(services == 4);
  memory.back() = 0xfffffffe;
  assert(!Storage::isErased(0, memory.size() * 4, read, service));
  memory.back() = 0xffffffffu;
  memory[0] = 0;
  assert(!Storage::isErased(0, memory.size() * 4, read, service));
  assert(!Storage::isErased(1, 8, read, service));
  assert(!Storage::isErased(4, 4, read, service));
  memory[0] = 0xffffffffu;
  assert(!Storage::isErased(0, (memory.size() + 1) * 4, read, service));
}
