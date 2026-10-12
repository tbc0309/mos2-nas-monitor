#pragma once
#include <stdint.h>
namespace CapabilityPlan {
enum class Cpu : uint8_t { RawCounters, ProcessorTable, Unsupported };
enum class Memory : uint8_t { Available, FreeAndCache, HostRam, Unsupported };
enum class Network : uint8_t { Counter64, Counter32, Unsupported };
enum class Temperature : uint8_t { SensorTable, Synology, CpuExtension, Unsupported };
struct Plan {
  Cpu cpu=Cpu::RawCounters;
  Memory memory=Memory::Available;
  Network network=Network::Counter64;
  Temperature temperature=Temperature::SensorTable;
  void reset() { *this=Plan(); }
  // Only a validated unsupported-object response advances these plans.
  // Network timeouts/authentication errors remain scheduler failures.
  bool fallbackCpu() {
    if (cpu==Cpu::RawCounters) { cpu=Cpu::ProcessorTable;return true; }
    cpu=Cpu::Unsupported;return false;
  }
  bool fallbackMemory() {
    if (memory==Memory::Available) { memory=Memory::FreeAndCache;return true; }
    if (memory==Memory::FreeAndCache) { memory=Memory::HostRam;return true; }
    memory=Memory::Unsupported;return false;
  }
  bool fallbackNetwork() {
    if (network==Network::Counter64) { network=Network::Counter32;return true; }
    network=Network::Unsupported;return false;
  }
};
inline bool numeric(uint8_t tag) { return tag==2 || tag==0x41 || tag==0x42 || tag==0x43 || tag==0x46; }
inline bool unsupported(uint8_t tag) { return tag>=0x80 && tag<=0x82; }
inline bool delta(uint64_t current,uint64_t previous,bool narrow,uint64_t &result) {
  if (narrow) { result=uint32_t(current)-uint32_t(previous); return result<=0x7fffffffU; }
  if (current<previous) return false;
  result=current-previous;return true;
}
}
