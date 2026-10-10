#pragma once
// Nonblocking SNMP sampling and weather polling; static metadata is cached for five minutes.
String oidFor(unsigned p) {
  switch (p) {
  case 0:
    return "1.3.6.1.4.1.2021.11.50.0";
  case 13:
    return "1.3.6.1.4.1.2021.4.14.0";
  case 14:
    return "1.3.6.1.4.1.2021.4.15.0";
  case 15:
    return "1.3.6.1.4.1.2021.11.51.0";
  case 16:
    return "1.3.6.1.4.1.2021.11.52.0";
  case 17:
    return "1.3.6.1.4.1.2021.11.53.0";
  case 18:
    return "1.3.6.1.4.1.2021.11.54.0";
  case 19:
    return "1.3.6.1.4.1.2021.11.56.0";
  case 20:
    return "1.3.6.1.4.1.2021.11.61.0";
  case 21:
    return "1.3.6.1.4.1.2021.11.64.0";
  case 1:
    return "1.3.6.1.4.1.2021.4.5.0";
  case 2:
    return "1.3.6.1.4.1.2021.4.6.0";
  case 3:
    return "1.3.6.1.4.1.6574.1.2.0";
  case 4:
    return "1.3.6.1.4.1.6574.1.5.1.0";
  case 5:
    return "1.3.6.1.2.1.1.3.0";
  case 6:
    return "1.3.6.1.2.1.25.2.3.1.4." + storageIndex;
  case 7:
    return "1.3.6.1.2.1.25.2.3.1.5." + storageIndex;
  case 8:
    return "1.3.6.1.2.1.25.2.3.1.6." + storageIndex;
  case 9:
    return "1.3.6.1.2.1.31.1.1.1.6." + interfaceIndex;
  case 10:
    return "1.3.6.1.2.1.31.1.1.1.10." + interfaceIndex;
  case 11:
    return storageBase + "." + storageIndex;
  default:
    return "1.3.6.1.2.1.2.2.1.2." + interfaceIndex;
  }
}
uint64_t rxNow = 0, txNow = 0;
bool rxValid = false, txValid = false;
void value(unsigned p, const Snmp::Value &v) {
  if (v.negative)
    return;
  switch (p) {
  case 0:
    cpuTicks.put(0, v.num);
    break;
  case 13:
    ramBuffers = v.num;
    bufferValid = true;
    break;
  case 14:
    ramCache = v.num;
    cacheValid = true;
    break;
  case 15:
  case 16:
  case 17:
  case 18:
  case 19:
  case 20:
  case 21:
    cpuTicks.put(p - 14, v.num);
    break;
  case 1:
    metric.total = v.num;
    break;
  case 2:
    ramFree = v.num;
    freeValid = true;
    break;
  case 3:
    metric.temp = v.num;
    break;
  case 4:
    metric.model = v.text;
    break;
  case 5:
    if (v.num < metric.uptime)
      cpuTicks.reset();
    metric.uptime = v.num;
    break;
  case 6:
    metric.units = v.num;
    break;
  case 7:
    metric.size = v.num;
    break;
  case 8:
    metric.used = v.num;
    if (metric.size && v.num <= metric.size)
      metric.storage = 100.0 * v.num / metric.size;
    break;
  case 9:
    rxNow = v.num;
    rxValid = true;
    break;
  case 10:
    txNow = v.num;
    txValid = true;
    break;
  case 11:
    metric.volume = v.text;
    break;
  case 12:
    metric.iface = v.text;
    break;
  }
}
void monitor() {
  static bool linkReady = false;
  static IPAddress boundIp;
  static bool checkingLink = false;
  static String candidateInterface;
  if (ap || WiFi.status() != WL_CONNECTED) {
    linkReady = false;
    return;
  }
  if (!linkReady || boundIp != WiFi.localIP()) {
    checkingLink = false;
    candidateInterface = "";
    snmp.begin();
    nasExtra.reset();
    nasExtra.begin();
    networkInterfaces.reset();
    linkReady = true;
    boundIp = WiFi.localIP();
    phase = 0;
    cycleAt = 0;
    missed = 0;
    discover = true;
    walkSteps = 0;
    walkOid = storageBase;
    metric.online = false;
    metric.counters = false;
    cpuTicks.reset();
    freeValid = bufferValid = cacheValid = false;
  }
  static uint32_t metadataAt = 0;
  static bool metadataCycle = true;
  uint32_t now = millis();
  if (phase == 0 && !snmp.busy())
    metadataCycle = !metadataAt || now - metadataAt >= 300000;
  Snmp::Value v;
  int result = snmp.poll(v);
  IPAddress ip;
  ip.fromString(cfg.nas);
  if (discover) {
    if (result) {
      // ifType=ethernet also includes disconnected ports. Check operStatus
      // before selecting a counter source; otherwise an unused NIC reports zero forever.
      if (checkingLink) {
        checkingLink = false;
        if (result == 1 && v.num == 1 && !v.negative) {
          interfaceIndex = candidateInterface;
          discover = false;
          phase = 0;
          cycleAt = 0;
        }
      } else if (result == 1 &&
                 v.oid.startsWith(phase == 0 ? storageBase + "." : ifaceBase + ".")) {
        String index = v.oid.substring(v.oid.lastIndexOf('.') + 1);
        if (phase == 0 && v.text.startsWith("/volume") && v.text.indexOf('/', 1) < 0) {
          storageIndex = index;
          metric.volume = v.text;
          phase = 1;
          walkOid = ifaceBase;
          walkSteps = 0;
        } else if (phase == 1 && v.num == 6) {
          candidateInterface = index;
          walkOid = v.oid;
          checkingLink = true;
        } else {
          walkOid = v.oid;
          if (++walkSteps > 64) {
            phase++;
            walkSteps = 0;
            walkOid = ifaceBase;
          }
        }
      } else {
        phase++;
        walkSteps = 0;
        walkOid = ifaceBase;
      }
      if (phase >= 2) {
        discover = false;
        phase = 0;
        cycleAt = 0;
      }
    }
    if (discover && !snmp.busy()) {
      if (phase == 0 && storageIndex.length()) {
        phase = 1;
        walkOid = ifaceBase;
      }
      if (phase == 1 && interfaceIndex.length()) {
        discover = false;
        phase = 0;
        return;
      }
      if (checkingLink)
        snmp.query(ip, cfg.port, cfg.community, "1.3.6.1.2.1.2.2.1.8." + candidateInterface);
      else
        snmp.query(ip, cfg.port, cfg.community, walkOid, true);
    }
    return;
  }
  if (result) {
    if (result == 1) {
      value(phase, v);
      if (freeValid && bufferValid && cacheValid && metric.total) {
        metric.memory = Resources::memory(metric.total, ramFree, ramBuffers, ramCache);
        metric.avail = min(metric.total, ramFree + ramBuffers + ramCache);
      }
      metric.last = now;
      metric.online = true;
      missed = 0;
    } else {
      missed++;
      switch (phase) {
      case 0:
        metric.cpu = -1;
        break;
      case 1:
        metric.total = 0;
        metric.memory = -1;
        break;
      case 2:
        metric.memory = -1;
        break;
      case 3:
        metric.temp = -1;
        break;
      case 7:
        metric.size = 0;
        metric.storage = -1;
        break;
      case 8:
        metric.storage = -1;
        break;
      }
    }
    phase++;
  }
  if (phase >= 22) {
    metric.cpu = cpuTicks.finish();
    metric.memory = freeValid && bufferValid && cacheValid
                        ? Resources::memory(metric.total, ramFree, ramBuffers, ramCache)
                        : -1;
    metric.avail = ramFree + ramBuffers + ramCache;
    if (metric.avail > metric.total)
      metric.avail = metric.total;
    freeValid = bufferValid = cacheValid = false;
    {
      if (rxValid && txValid) {
        if (metric.counters && now - metric.counterTime > 0) {
          float seconds = (now - metric.counterTime) / 1000.0;
          if (rxNow >= metric.oldRx && txNow >= metric.oldTx) {
            metric.rx = (rxNow - metric.oldRx) / seconds;
            metric.tx = (txNow - metric.oldTx) / seconds;
          } else
            metric.rx = metric.tx = -1;
        }
        metric.oldRx = rxNow;
        metric.oldTx = txNow;
        metric.counterTime = now;
        metric.counters = true;
      } else {
        metric.counters = false;
        metric.rx = metric.tx = -1;
      }
      sampleHistory();
    }
    if (metadataCycle)
      metadataAt = now;
    cycleAt = now;
    phase = 0;
    rxValid = txValid = false;
  }
  if (metric.online && now - metric.last > max(15000u, cfg.interval * 3000u)) {
    metric.online = false;
    metric.cpu = metric.memory = metric.storage = metric.temp = metric.rx = metric.tx = -1;
    metric.counters = false;
  }
  if (!snmp.busy() && phase == 0 && (!storageIndex.length() || !interfaceIndex.length()) &&
      now - discoveryAt > 5000) {
    discover = true;
    discoveryAt = now;
    walkOid = storageBase;
    walkSteps = 0;
    return;
  }
  if (!snmp.busy() && (cycleAt == 0 || now - cycleAt >= cfg.interval * 1000)) {
    while ((!metadataCycle &&
            ((phase == 1 && metric.total > 0) || (phase == 4 && metric.model.length()) ||
             (phase == 6 && metric.units > 0) || (phase == 7 && metric.size > 0) ||
             (phase == 11 && metric.volume.length()) || (phase == 12 && metric.iface.length()))) ||
           (((phase >= 6 && phase <= 8) || phase == 11) && !storageIndex.length()) ||
           ((phase == 9 || phase == 10 || phase == 12) && !interfaceIndex.length()))
      phase++;
    if (phase < 22)
      snmp.query(ip, cfg.port, cfg.community, oidFor(phase));
  }
}

void weatherMonitor() {
  if (!cfg.weatherEnabled || ap || WiFi.status() != WL_CONNECTED)
    return;
  uint32_t now = millis();
  int result = weatherClient.poll(weather);
  if (result)
    Serial.printf("WEATHER result=%d city=%s temp=%.1f humidity=%.0f error=%s heap=%u\n", result,
                  weather.cityEnglish.c_str(), weather.temperature, weather.humidity,
                  weather.error.c_str(), ESP.getFreeHeap());
  uint32_t interval = weather.error.length() ? 60000 : cfg.weatherInterval * 60000;
  if (!weatherClient.busy() && (!weatherStarted || now - weatherAttempt >= interval)) {
    weatherStarted = true;
    weatherAttempt = now;
    if (!weatherClient.query(cfg.cityCode))
      weather.error = "Weather connection failed";
  }
}
