#pragma once
String weatherStatus(uint32_t now = millis()) {
  return "{\"enabled\":" + String(cfg.weatherEnabled ? "true" : "false") +
         ",\"valid\":" + String(weather.valid ? "true" : "false") +
         ",\"city\":" + jsonString(weather.city) +
         ",\"condition\":" + jsonString(weather.condition) +
         ",\"cityEnglish\":" + jsonString(weather.cityEnglish) +
         ",\"conditionEnglish\":" + jsonString(weather.conditionEnglish) +
         ",\"temperature\":" + String(weather.temperature) +
         ",\"humidity\":" + String(weather.humidity) + ",\"aqi\":" + String(weather.aqi) +
         ",\"observed\":" + jsonString(weather.observed) +
         ",\"error\":" + jsonString(weather.error) +
         ",\"age\":" + String(weather.valid ? (now - weather.fetched) / 1000 : 0) + "}";
}
String diskStatus();
String volumeStatus();
String alarmStatus();
String interfacesStatus() {
  networkInterfaces.request();
  String result = "[";
  for (unsigned i = 0; i < networkInterfaces.count; i++) {
    const auto &entry = networkInterfaces.entries[i];
    if (i)
      result += ",";
    result += "{\"index\":" + jsonString(entry.index) + ",\"name\":" + jsonString(entry.name) +
              ",\"up\":" + (entry.up ? "true" : "false") + "}";
  }
  return result + "]";
}
void status() {
  if (!authorized())
    return;
  const uint32_t now = millis();
  const unsigned heap = ESP.getFreeHeap();
  auto emit = [&](Print &out) {
    auto append = [&](const String &part) { out.print(part); };
    append("{\"mode\":");
    append(jsonString("snmp"));
    append(",\"page\":");
    append(String(page));
    append(",\"online\":");
    append(String(metric.online ? "true" : "false"));
    append(",\"cpu\":");
    append(String(metric.cpu));
    append(",\"memoryTotal\":");
    append(String(metric.total * 1024));
    append(",\"memoryAvail\":");
    append(String(metric.avail * 1024));
    append(",\"memory\":");
    append(String(metric.memory));
    append(",\"storage\":");
    append(String(metric.storage));
    append(",\"temperature\":");
    append(String(metric.temp));
    append(",\"rx\":");
    append(String(metric.rx));
    append(",\"tx\":");
    append(String(metric.tx));
    append(",\"volume\":");
    append(jsonString(metric.volume));
    append(",\"interface\":");
    append(jsonString(metric.iface));
    append(",\"storageIndex\":");
    append(jsonString(storageIndex));
    append(",\"interfaceIndex\":");
    append(jsonString(interfaceIndex));
    append(",\"interfaces\":");
    append(interfacesStatus());
    append(",\"model\":");
    append(jsonString(metric.model));
    append(",\"displayName\":");
    append(jsonString(machines[activeMachine].name.length()?machines[activeMachine].name:automaticDeviceName()));
    append(",\"wifiConnected\":");
    append(String(WiFi.status() == WL_CONNECTED ? "true" : "false"));
    append(",\"wifiSsid\":");
    append(jsonString(WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String()));
    append(",\"ip\":");
    append(jsonString(WiFi.localIP().toString()));
    append(",\"freeHeap\":");
    append(String(heap));
    append(",\"clock\":");
    append(jsonString(screen.clock));
    append(",\"date\":");
    append(jsonString(screen.date));
    append(",\"weather\":");
    append(weatherStatus(now));
    append(",\"disks\":");
    append(diskStatus());
    append(",\"diskUpdated\":");
    append(String(storageCollector.diskUpdated));
    append(",\"volumeUpdated\":");
    append(String(storageCollector.volumeUpdated));
    append(",\"volumes\":");
    append(volumeStatus());
    append(",\"alarm\":");
    append(alarmStatus());
    append(",\"albumCount\":");
    append(String(albumCount));
    append(",\"albumDraws\":");
    append(String(albumDraws));
    append(",\"albumFixed\":");
    append(String(cfg.photoFixedMode));
    append(",\"albumSlot\":");
    append(String(albumCount ? albumSlots[albumPosition] : -1));
    append(",\"diskGroup\":");
    append(String(screen.diskGroup));
    append(",\"volumeGroup\":");
    append(String(screen.volumeGroup));
    append(",\"resetPending\":");
    append(String(resetConfirm.pending ? "true" : "false"));
    append(",\"language\":");
    append(jsonString(cfg.displayLanguage));
    append("}");
  };
  ConfigCountSink count; emit(count);
  web.setContentLength(count.count); web.send(200, "application/json", "");
  ConfigWebSink out; emit(out); out.flush();
}
