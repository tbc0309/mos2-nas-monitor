#pragma once
namespace Telemetry {
uint32_t period(unsigned group) {
  return group == Identity ? Sampling::IdentityPeriod : group == Uptime ? Sampling::UptimePeriod : group == Temperature ? Sampling::TemperaturePeriod :
         (group == Cpu || group == Memory) ? max(15000u, cfg.snmpPollIntervalSeconds * 1000u) : cfg.snmpPollIntervalSeconds * 1000u;
}
void reset() {
  startedAt = millis();
  storageReleased = false;
  uptimeAt = 0;
  resetCapabilities();
  startup.reset(startedAt, page);
  for (auto &job : collectors) {
    job.client.begin();
    job.schedule = {};
    job.retry = {};
    job.active = false;
    job.updated = 0;
    job.published = false;
    job.cursor = 0;
  }
  metric = Metrics();
  cpuTicks.reset();
}
bool commit(unsigned group, uint32_t now) {
  auto &job = collectors[group];
  auto *n = job.numbers;
  switch (group) {
  case Cpu: {
    if (capabilities.cpu == CapabilityPlan::Cpu::ProcessorTable) {
      if (!job.tableRows) return false;
      metric.cpu = float(n[0]) / job.tableRows; metric.cpuAt = now; break;
    }
    for (unsigned i = 0; i < 8; i++)
      cpuTicks.put(i, n[i]);
    float cpu = cpuTicks.finish();
    if (cpu >= 0) {
      metric.cpu = cpu;
      metric.cpuAt = now;
    }
    break;
  }
  case Memory: {
    if (capabilities.memory == CapabilityPlan::Memory::HostRam) {
      if (!n[0] || !n[1] || n[2] > n[1] || n[1] > UINT64_MAX/n[0]) return false;
      metric.total=n[0]*n[1]/1024; metric.avail=n[0]*(n[1]-n[2])/1024;
      metric.memory=100.0*double(n[2])/n[1];
    } else {
      if (!n[0] || n[1] > n[0]) return false;
      metric.total=n[0];
      metric.avail=n[1];
      if (capabilities.memory==CapabilityPlan::Memory::FreeAndCache) {
        // Saturating addition also protects against hostile 64-bit counter values.
        metric.avail+=min(n[0]-metric.avail,n[2]);
        metric.avail+=min(n[0]-metric.avail,n[3]);
      }
      metric.memory=100.0*double(metric.total-metric.avail)/metric.total;
    }
    metric.memoryAt=now; break;
  }
  case Network: {
    uint64_t received=0,transmitted=0;
    const bool narrow=capabilities.network==CapabilityPlan::Network::Counter32;
    if (metric.counters && now!=metric.counterTime &&
        CapabilityPlan::delta(n[0],metric.oldRx,narrow,received) &&
        CapabilityPlan::delta(n[1],metric.oldTx,narrow,transmitted)) {
      const float seconds=uint32_t(now-metric.counterTime)/1000.0f;
      metric.rx=received/seconds;metric.tx=transmitted/seconds;
      metric.networkAt=now;sampleHistory();
    } else metric.rx=metric.tx=-1;
    metric.oldRx=n[0];metric.oldTx=n[1];metric.counterTime=now;metric.counters=true;
    break;
  }
  case Temperature:
    metric.temp = capabilities.temperature == CapabilityPlan::Temperature::Synology ? n[0] : n[0] / 1000.0f;
    metric.tempAt = now;
    break;
  case Uptime:
    if (n[0] < metric.uptime) {
      cpuTicks.reset();
      metric.counters = false;
    }
    metric.uptime = n[0];
    uptimeAt = now;
    break;
  default:
    break;
  }
  bool valid = group == Cpu ? metric.cpu >= 0 : group == Network ? metric.rx >= 0 : true;
  if (valid && !job.published) {
    Serial.printf("SAMPLE first group=%s at=%u\n", names[group], now);
    job.published = true;
  }
  job.updated = now;
  return true;
}
void expire(uint32_t now) {
  if (metric.uptime) {
    uint32_t ticks = uint32_t(now - uptimeAt) / 10;
    metric.uptime += ticks;
    uptimeAt += ticks * 10;
  }
  const uint32_t ttl = max(Sampling::LiveMinimumTtl, cfg.snmpPollIntervalSeconds * 6000u);
  const uint32_t cacheTtl = max(60000u, ttl);
  if (Resources::expireSample(now, metric.cpuAt, cacheTtl))
    metric.cpu = -1;
  if (Resources::expireSample(now, metric.memoryAt, cacheTtl))
    metric.memory = -1;
  if (Resources::expireSample(now, metric.tempAt, max(cacheTtl, Sampling::TemperaturePeriod * 4)))
    metric.temp = -1;
  if (Resources::expireSample(now, metric.networkAt, cacheTtl)) {
    metric.rx = metric.tx = -1;
    metric.counters = false;
  }
  // Reachability and field validity are separate: one reachable OID does not
  // make another group's old value fresh.
  metric.online = metric.last && now - metric.last <= ttl;
}
bool storageReady(uint32_t now) {
  unsigned mask = 0;
  for (unsigned i = 0; i < 4; ++i)
    if (collectors[i].published || !supported(i)) mask |= 1u << i;
  storageReleased |= Sampling::storageBootstrapReady(now - startedAt, mask, page);
  return storageReleased;
}
// Append to the caller's bounded JSON sink; no temporary diagnostics document.
void emitDiagnostics(JsonFields &root, Print &out, uint32_t now) {
  uint32_t sent = 0, received = 0, timeouts = 0, invalid = 0, errors = 0;
  bool pending = false, bound = true;
  for (unsigned i = 0; i < Count; ++i) {
    const auto &c = collectors[i].client;
    sent += c.sent; received += c.received; timeouts += c.timeouts;
    invalid += c.invalid; errors += c.sendErrors;
    pending |= c.busy(); bound &= c.bound;
  }
  root.field("systemType", String(DeviceProfile::name(deviceProfile.system)));
  root.field("systemEvidence", unsigned(deviceProfile.evidence));
  root.field("sent", sent); root.field("received", received);
  root.field("timeouts", timeouts); root.field("invalid", invalid);
  root.field("sendErrors", errors); root.field("pending", pending); root.field("bound", bound);
  root.field("snmpVersion", cfg.snmpVersion);
  root.field("securityError", Snmp::Usm::session().error);
  root.key("collectors"); JsonFields groups(out);
  for (unsigned i = 0; i < Count; ++i) {
    const auto &job = collectors[i];
    groups.key(names[i]); JsonFields entry(out);
    entry.field("active", job.active); entry.field("failures", job.schedule.failures);
    entry.key("ageMs");
    if (job.updated) out.print(uint32_t(now - job.updated)); else out.print("null");
    entry.finish();
  }
  groups.finish();
}
}
