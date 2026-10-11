#pragma once
#include "poll_schedule.h"
#include "startup_plan.h"

// Six independent collectors publish coherent snapshots into Metrics. Rendering
// only reads Metrics; no page change, language change or HTTP read starts an OID.
namespace Telemetry {
uint32_t startedAt = 0;
Sampling::StartupPlan startup;
bool storageReleased = false;
uint32_t uptimeAt = 0;

enum Group { Cpu, Memory, Network, Temperature, Uptime, Identity, Count };
const char *const names[Count] = {"cpu", "memory", "network", "temperature", "uptime", "identity"};
const unsigned fields[Count][8] = {
    {0, 15, 16, 17, 18, 19, 20, 21}, {1, 2, 13, 14}, {9, 10}, {3}, {5}, {4}};
const unsigned lengths[Count] = {8, 4, 2, 1, 1, 1};
struct Collector {
  Snmp::Client client;
  Sampling::Schedule schedule;
  Sampling::OidRetry retry;
  uint64_t numbers[8] = {};
  unsigned cursor = 0;
  bool active = false, published = false;
  uint32_t updated = 0, cycleStarted = 0;
} collectors[Count];

String oid(unsigned field) {
  switch (field) {
  case 0:
    return "1.3.6.1.4.1.2021.11.50.0";
  case 15:
  case 16:
  case 17:
  case 18:
  case 19:
  case 20:
  case 21: {
    const unsigned n[] = {51, 52, 53, 54, 56, 61, 64};
    return "1.3.6.1.4.1.2021.11." + String(n[field - 15]) + ".0";
  }
  case 1:
    return "1.3.6.1.4.1.2021.4.5.0";
  case 2:
    return "1.3.6.1.4.1.2021.4.6.0";
  case 13:
    return "1.3.6.1.4.1.2021.4.14.0";
  case 14:
    return "1.3.6.1.4.1.2021.4.15.0";
  case 9:
    return "1.3.6.1.2.1.31.1.1.1.6." + interfaceIndex;
  case 10:
    return "1.3.6.1.2.1.31.1.1.1.10." + interfaceIndex;
  case 3:
    return "1.3.6.1.4.1.6574.1.2.0";
  case 5:
    return "1.3.6.1.2.1.25.1.1.0";
  default:
    return "1.3.6.1.4.1.6574.1.5.1.0";
  }
}
uint32_t period(unsigned group) {
  return group == Identity ? Sampling::IdentityPeriod : group == Uptime ? Sampling::UptimePeriod : group == Temperature ? Sampling::TemperaturePeriod :
         (group == Cpu || group == Memory) ? max(15000u, cfg.interval * 1000u) : cfg.interval * 1000u;
}
void reset() {
  startedAt = millis();
  storageReleased = false;
  uptimeAt = 0;
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
    for (unsigned i = 0; i < 8; i++)
      cpuTicks.put(i, n[i]);
    float cpu = cpuTicks.finish();
    if (cpu >= 0) {
      metric.cpu = cpu;
      metric.cpuAt = now;
    }
    break;
  }
  case Memory:
    if (!n[0] || n[1] > n[0])
      return false;
    metric.total = n[0];
    metric.avail = min(n[0], n[1] + n[2] + n[3]);
    metric.memory = Resources::memory(n[0], n[1], n[2], n[3]);
    metric.memoryAt = now;
    break;
  case Network:
    if (metric.counters && now != metric.counterTime && n[0] >= metric.oldRx &&
        n[1] >= metric.oldTx) {
      float seconds = uint32_t(now - metric.counterTime) / 1000.0f;
      metric.rx = (n[0] - metric.oldRx) / seconds;
      metric.tx = (n[1] - metric.oldTx) / seconds;
      metric.networkAt = now;
      sampleHistory();
    } else {
      metric.rx = metric.tx = -1;
    }
    metric.oldRx = n[0];
    metric.oldTx = n[1];
    metric.counterTime = now;
    metric.counters = true;
    break;
  case Temperature:
    metric.temp = n[0];
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
  const uint32_t ttl = max(Sampling::LiveMinimumTtl, cfg.interval * 6000u);
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
    if (collectors[i].published) mask |= 1u << i;
  storageReleased |= Sampling::storageBootstrapReady(now - startedAt, mask, page);
  return storageReleased;
}
String diagnostics() {
  uint32_t sent = 0, received = 0, timeouts = 0, invalid = 0, errors = 0;
  bool pending = false, bound = true;
  String groups = "\"collectors\":{";
  uint32_t now = millis();
  for (unsigned i = 0; i < Count; i++) {
    auto &job = collectors[i];
    auto &c = job.client;
    sent += c.sent;
    received += c.received;
    timeouts += c.timeouts;
    invalid += c.invalid;
    errors += c.sendErrors;
    pending |= c.busy();
    bound &= c.bound;
    if (i)
      groups += ',';
    groups += String('"') + names[i] + "\":{\"active\":" + (job.active ? "true" : "false") +
              ",\"failures\":" + job.schedule.failures +
              ",\"ageMs\":" + (job.updated ? String(uint32_t(now - job.updated)) : String("null")) +
              "}";
  }
  return "\"sent\":" + String(sent) + ",\"received\":" + received + ",\"timeouts\":" + timeouts +
         ",\"invalid\":" + invalid + ",\"sendErrors\":" + errors +
         ",\"pending\":" + (pending ? "true" : "false") +
         ",\"bound\":" + (bound ? "true" : "false") +
         ",\"snmpVersion\":" + jsonString(cfg.snmpVersion) +
         ",\"securityError\":" + jsonString(Snmp::Usm::session().error) + "," + groups + "}";
}
} // namespace Telemetry

void monitor() {
  static bool ready = false;
  static IPAddress boundIp;
  static String target;
  static uint32_t interfacesAt = 0;
  static unsigned first = 0;
  if (ap || WiFi.status() != WL_CONNECTED) {
    Telemetry::expire(millis());
    nasExtra.expire(millis());
    ready = false;
    return;
  }
  String key = cfg.nas + "/" + cfg.port + "/" + cfg.community + "/" + cfg.snmpVersion + "/" +
               cfg.snmpUser + "/" + cfg.snmpPassword + "/" + cfg.snmpAuth + "/" + cfg.snmpPrivacy +
               "/" + cfg.snmpPrivacyPassword;
  if (!ready || boundIp != WiFi.localIP() || target != key) {
    Snmp::Usm::session().configure(cfg.snmpVersion == "3", cfg.snmpUser, cfg.snmpPassword,
                                   cfg.snmpPrivacy, cfg.snmpPrivacyPassword,
                                   (uint64_t(ESP.random()) << 32) | ESP.random(), cfg.snmpAuth);
    Telemetry::reset();
    nasExtra.reset();
    nasExtra.begin();
    networkInterfaces.reset();
    storageIndex = cfg.storage;
    interfaceIndex = cfg.iface;
    ready = true;
    boundIp = WiFi.localIP();
    target = key;
    interfacesAt = 0;
    Serial.printf("SAMPLING start at=%lu\n", millis());
  }
  Telemetry::startup.promote(millis(), page);
  Snmp::Usm::session().step();
  uint32_t now = millis();
  IPAddress ip;
  ip.fromString(cfg.nas);
  if (Telemetry::startup.ready(now, 8) &&
      (!interfacesAt || now - interfacesAt >= Sampling::InterfacePeriod)) {
    networkInterfaces.request();
    interfacesAt = now;
  }
  // Interface discovery publishes each checked port immediately. It never
  // pauses CPU, RAM or temperature collectors, even if the NAS has many bridges.
  String selected = cfg.iface;
  if (!selected.length()) {
    // Prefer physical Synology ports over OVS/bridge interfaces.
    for (unsigned i = 0; i < networkInterfaces.count; i++) {
      auto &entry = networkInterfaces.entries[i];
      if (entry.up && (entry.name.startsWith("eth") || entry.name.startsWith("en"))) {
        selected = entry.index;
        break;
      }
    }
    if (!selected.length()) {
      for (unsigned i = 0; i < networkInterfaces.count; i++)
        if (networkInterfaces.entries[i].up) {
          selected = networkInterfaces.entries[i].index;
          break;
        }
    }
  }
  for (unsigned i = 0; i < networkInterfaces.count; i++)
    if (networkInterfaces.entries[i].index == selected)
      metric.iface = networkInterfaces.entries[i].name;
  if (selected.length() && selected != interfaceIndex) {
    auto &job = Telemetry::collectors[Telemetry::Network];
    job.client.cancel();
    job.active = false;
    job.schedule = {};
    job.retry = {};
    interfaceIndex = selected;
    metric.counters = false;
    metric.networkAt = 0;
    metric.rx = metric.tx = -1;
  }
  discover = !interfaceIndex.length();
  // The volume collector is the sole owner of storage sampling; reuse its
  // coherent cache instead of querying the same volume twice.
  static String storageSelection;
  if (storageSelection != cfg.storage) {
    storageSelection = cfg.storage;
    storageIndex = cfg.storage;
    metric.storage = -1;
  }
  for (unsigned i = 0; i < nasExtra.volumeCount; i++) {
    auto &v = nasExtra.volumes[i];
    if ((!storageIndex.length() && !cfg.storage.length()) || v.index == storageIndex) {
      storageIndex = v.index;
      metric.volume = v.name;
      metric.units = v.units;
      metric.size = v.size;
      metric.used = v.used;
      metric.storage = v.valid && v.size && v.used <= v.size ? 100.0 * double(v.used) / v.size : -1;
      break;
    }
  }
  for (unsigned offset = 0; offset < Telemetry::Count; offset++) {
    unsigned group = (first + offset) % Telemetry::Count;
    auto &job = Telemetry::collectors[group];
    if (!Telemetry::startup.ready(now, group)) continue;
    if (group == Telemetry::Network && !interfaceIndex.length())
      continue;
    if (!job.active && job.schedule.ready(now)) {
      job.active = true;
      job.cursor = 0;
      job.cycleStarted = now;
      job.retry = {};
    }
    if (!job.active)
      continue;
    Snmp::Value v;
    uint32_t beforeTimeouts = job.client.timeouts;
    int result = job.client.poll(v);
    if (result < 0 && job.client.timeouts != beforeTimeouts && job.retry.timeout(now))
      result = 0; // Keep this OID's cursor and the rest of the coherent snapshot.
    if (result == 1) job.retry.success();
    if (result) {
      bool ok = result == 1 && !v.negative &&
                (group == Telemetry::Identity ? v.type == 4 && v.text.length() : v.type != 4);
      if (ok) {
        job.numbers[job.cursor] = v.num;
        if (group == Telemetry::Identity)
          metric.model = v.text;
        metric.last = now;
      }
      if (!ok || ++job.cursor == Telemetry::lengths[group]) {
        if (ok)
          ok = Telemetry::commit(group, now);
        // Counter-based rates need a second snapshot; prime them after one second.
        uint32_t wait = Telemetry::period(group);
        if (ok && ((group == Telemetry::Cpu && metric.cpu < 0) ||
                   (group == Telemetry::Network && metric.rx < 0))) wait = Sampling::CounterPrime;
        if (!ok && !job.updated)
          Serial.printf("SAMPLE group=%s failed field=%u at=%u\n", Telemetry::names[group], job.cursor, now);
        if (ok && wait == Sampling::CounterPrime)
          job.schedule.finish(now, true, wait);
        else
          job.schedule.finishCycle(now, ok, wait, job.cycleStarted);
        job.active = false;
      }
    }
    if (job.active && !job.client.busy() && job.retry.ready(now))
      job.client.query(ip, cfg.port, cfg.community,
                       Telemetry::oid(Telemetry::fields[group][job.cursor]));
  }
  first = (first + 1) % Telemetry::Count;
  phase = first; // Diagnostics now identifies a collector, rather than a legacy OID phase.
  Telemetry::expire(now);
}
