#pragma once
namespace Telemetry {
uint32_t startedAt = 0;
Sampling::StartupPlan startup;
bool storageReleased = false;
uint32_t uptimeAt = 0;

enum Group { Cpu, Memory, Network, Temperature, Uptime, Identity, Count };
const char *const names[Count] = {"cpu", "memory", "network", "temperature", "uptime", "identity"};
struct Collector {
  Snmp::Client client;
  Sampling::Schedule schedule;
  Sampling::OidRetry retry;
  uint64_t numbers[8] = {};
  unsigned cursor = 0, tableRows = 0;
  String walk;
  bool active = false, published = false;
  uint32_t updated = 0, cycleStarted = 0;
} collectors[Count];

}
