#include <cassert>
#include "poll_schedule.h"
int main() {
  static_assert(Sampling::DiskPeriod == 3600000, "hourly disk sampling");
  static_assert(Sampling::VolumePeriod == 7200000, "two-hour volume sampling");
  static_assert(Sampling::DiskTtl > Sampling::DiskPeriod, "disk cache covers refresh");
  static_assert(Sampling::VolumeTtl > Sampling::VolumePeriod, "volume cache covers refresh");
  Sampling::OidRetry retry;
  assert(retry.timeout(100) && !retry.ready(599) && retry.ready(600));
  assert(!retry.timeout(700));
  retry.success(); assert(retry.timeout(0xffffff00) && retry.ready(244));
  Sampling::Schedule cycle;
  cycle.finishCycle(3500, true, 5000, 1000);
  assert(cycle.due == 6000); // Period includes collection time.
  cycle.finishCycle(9000, true, 5000, 1000);
  assert(cycle.due == 10000); // No tight loop after an overrun.
  cycle.finishCycle(0x20, true, 1000, 0xfffffff0);
  assert(cycle.due == 1032); // Wrap-safe and minimum inter-cycle gap.
  Sampling::Schedule cpu, memory;
  assert(cpu.ready(0) && memory.ready(0));
  cpu.finish(100, false, 5000);
  memory.finish(100, true, 5000);
  cpu.finish(5100, false, 5000);
  assert(!cpu.ready(10100) && memory.ready(5100));
  for (unsigned i = 0; i < 8; i++) cpu.finish(100, false, 5000);
  assert(cpu.due == 100 + Sampling::RetryCeiling); // Failed groups have bounded retries.
  cpu.finish(100, true, 5000);
  assert(cpu.failures == 0 && cpu.due == 5100);
  memory.finish(0xfffffff0, true, 32);
  assert(!memory.ready(0xf) && memory.ready(0x10));
}
