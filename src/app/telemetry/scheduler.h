#pragma once
void monitor() {
  static bool ready = false, suspended = false;
  if (!activity.snmp(millis())) {
    if (!suspended) {
      for (auto &job : Telemetry::collectors) { job.client.cancel(); job.active = false; }
      storageCollector.suspend(); networkInterfaces.suspend();
      suspended = true;
    }
    return; // Keep last successful snapshots; they remain visible on re-entry.
  }
  if (suspended) {
    suspended = false;
    metric.counters = false;
    for (auto &job : Telemetry::collectors) if (!job.schedule.failures) job.schedule.initialized = false;
  }
  static IPAddress boundIp;
  static uint32_t target = 0;
  static uint32_t interfacesAt = 0;
  static unsigned first = 0;
  static int prioritizedPage = -1;
  if (int(page) != prioritizedPage) {
    prioritizedPage = page;
    first = page == 1 ? Telemetry::Network : page == 4 ? Telemetry::Identity : Telemetry::Cpu;
  }
  if (ap || WiFi.status() != WL_CONNECTED) {
    Telemetry::expire(millis());
    storageCollector.expire(millis());
    ready = false;
    return;
  }
  // Allocation-free configuration fingerprint; credentials never enter a temporary key.
  uint32_t key = 2166136261u;
  for (const String *value : {&cfg.snmpHost, &cfg.snmpCommunity, &cfg.snmpVersion, &cfg.snmpUsername,
                              &cfg.snmpAuthPassword, &cfg.snmpAuthProtocol, &cfg.snmpPrivacyProtocol, &cfg.snmpPrivacyPassword}) {
    for (unsigned i = 0; i < value->length(); ++i) key = (key ^ uint8_t((*value)[i])) * 16777619u;
    key = (key ^ 0xffu) * 16777619u;
  }
  key = (key ^ cfg.snmpPort) * 16777619u;
  key = (key ^ activeMachine) * 16777619u;
  if (!ready || boundIp != WiFi.localIP() || target != key) {
    Snmp::Usm::session().configure(cfg.snmpVersion == "3", cfg.snmpUsername, cfg.snmpAuthPassword,
                                   cfg.snmpPrivacyProtocol, cfg.snmpPrivacyPassword,
                                   (uint64_t(ESP.random()) << 32) | ESP.random(), cfg.snmpAuthProtocol);
    Telemetry::reset();
    storageCollector.reset();
    storageCollector.begin();
    networkInterfaces.reset();
    Snmp::Client::resetTransportBudget();
    storageIndex = cfg.storageVolumeIndex;
    interfaceIndex = cfg.networkInterfaceIndex;
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
  ip.fromString(cfg.snmpHost);
  if (Telemetry::startup.ready(now, 8) &&
      (!interfacesAt || now - interfacesAt >= Sampling::InterfacePeriod)) {
    networkInterfaces.request();
    interfacesAt = now;
  }
  // Interface discovery publishes each checked port immediately. It never
  // pauses CPU, RAM or temperature collectors, even if the NAS has many bridges.
  String selected = cfg.networkInterfaceIndex;
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
    Telemetry::capabilities.network = CapabilityPlan::Network::Counter64;
    metric.counters = false;
    metric.networkAt = 0;
    metric.rx = metric.tx = -1;
  }
  discover = !interfaceIndex.length();
  // The volume collector is the sole owner of storage sampling; reuse its
  // coherent cache instead of querying the same volume twice.
  static String storageSelection;
  if (storageSelection != cfg.storageVolumeIndex) {
    storageSelection = cfg.storageVolumeIndex;
    storageIndex = cfg.storageVolumeIndex;
    metric.storage = -1;
  }
  for (unsigned i = 0; i < storageCollector.volumeCount; i++) {
    auto &v = storageCollector.volumes[i];
    if ((!storageIndex.length() && !cfg.storageVolumeIndex.length()) || v.index == storageIndex) {
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
    if (!Telemetry::supported(group)) continue;
    if (group == Telemetry::Network && !interfaceIndex.length())
      continue;
    if (!job.active && job.schedule.ready(now)) {
      job.active = true;
      Telemetry::beginCycle(group, job);
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
      const int accepted=result==1?Telemetry::acceptValue(group,job,v):-1;
      if (result==1 && v.valid) metric.last=now;
      if (accepted) {
        bool ok=accepted>0 && Telemetry::commit(group,now);
        uint32_t wait=Telemetry::period(group);
        if (ok && ((group==Telemetry::Cpu && metric.cpu<0) ||
                   (group==Telemetry::Network && metric.rx<0))) wait=Sampling::CounterPrime;
        if (!ok && !job.updated) Serial.printf("SAMPLE group=%s failed field=%u at=%u\n",Telemetry::names[group],job.cursor,now);
        if (ok && wait==Sampling::CounterPrime) job.schedule.finish(now,true,wait);
        else job.schedule.finishCycle(now,ok,wait,job.cycleStarted);
        job.active=false;
      }
    }
    if (job.active && !job.client.busy() && job.retry.ready(now)) {
      if(group==Telemetry::Identity && job.cursor>=2+unsigned(Telemetry::identityNameFallback) &&
         Telemetry::probeSynologyModel()) {
        const unsigned liveMask=(Telemetry::collectors[Telemetry::Cpu].published?1u:0u) |
                                (Telemetry::collectors[Telemetry::Memory].published?2u:0u);
        if(!Sampling::optionalIdentityReady(now-Telemetry::startedAt,liveMask,page,
            SnmpAdapter::policy(Telemetry::deviceProfile).vendorDiskTable)) continue;
      }
      bool next=false;
      const String oid=Telemetry::queryOid(group,job,next);
      job.client.query(ip,cfg.snmpPort,cfg.snmpCommunity,oid,next);
    }
  }
  first = (first + 1) % Telemetry::Count;
  phase = first; // Diagnostics now identifies a collector, rather than a legacy OID phase.
  Telemetry::expire(now);
}
