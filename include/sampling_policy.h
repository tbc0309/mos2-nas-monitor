#pragma once
#include <stdint.h>
// Milliseconds. Network waits are nonblocking; cached validity is separate.
namespace Sampling {
constexpr uint32_t RequestTimeout = 4000, SendSpacing = 500;
constexpr unsigned MaxPending = 1;
constexpr uint32_t CounterPrime = 1000, MinimumCycleGap = 1000;
constexpr uint32_t RetryFloor = 15000, RetryCeiling = 120000;
constexpr uint32_t IdentityPeriod = 3600000, UptimePeriod = 7200000, TemperaturePeriod = 15000;
constexpr uint32_t InterfacePeriod = 300000, DiscoveryRetry = 5000;
constexpr uint32_t LiveMinimumTtl = 30000, DiskTtl = 7200000, VolumeTtl = 14400000;
constexpr uint32_t DiskTemperaturePeriod = 300000, DiskTemperatureTtl = 900000;
constexpr uint32_t DiskPeriod = 3600000, VolumePeriod = 7200000, TableTtl = 7200000;
constexpr uint32_t WeatherTimeout = 3500, WeatherRetry = 60000;
// One prompt retry, then progressively reduce load during sustained outages.
inline uint32_t weatherRetry(unsigned failures) {
  if (failures <= 1) return WeatherRetry;
  if (failures == 2) return 300000;
  if (failures == 3) return 600000;
  if (failures == 4) return 1200000;
  return 1800000;
}
}
