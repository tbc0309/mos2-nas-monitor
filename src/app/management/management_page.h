#pragma once
void startAP() {
  if (ap)
    return;
  weatherClient.cancel();
  ap = true;
  qrPage = false;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName.c_str(), apPass.c_str());
  dns.start(53, "*", WiFi.softAPIP());
  requestDraw();
}
// One serializer feeds API Strings and bounded HTML chunks; credentials are excluded.
void emitPublicConfig(Print &out, int selected = -1) {
  const unsigned edit = selected >= 0 && unsigned(selected) < machineCount ? unsigned(selected) : activeMachine;
  const auto &m = machines[edit];
  JsonFields root(out);
  root.key("machines"); out.write('[');
  for (unsigned i = 0; i < machineCount; ++i) {
    if (i) out.write(',');
    JsonFields profile(out);
    profile.field("name", machines[i].name);
    profile.field("address", machines[i].snmpHost);
    profile.finish();
  }
  out.write(']');
  root.field("activeMachine", activeMachine);
  root.field("selectedMachine", edit);
  root.field("machineName", m.name);
  root.field("ssid", cfg.wifiSsid);
  root.field("nas", m.snmpHost);
  root.field("snmpVersion", m.snmpVersion);
  root.field("snmpUser", m.snmpUsername);
  root.field("snmpAuth", m.snmpAuthProtocol);
  root.field("snmpPrivacy", m.snmpPrivacyProtocol);
  root.field("mode", String("snmp"));
  root.field("port", m.snmpPort);
  root.field("storage", m.storageVolumeIndex);
  root.field("iface", m.networkInterfaceIndex);
  root.field("home", cfg.startupPage);
  root.field("interval", m.snmpPollIntervalSeconds);
  root.field("brightness", cfg.displayBrightnessPercent);
  root.field("weatherEnabled", cfg.weatherEnabled);
  root.field("cityCode", cfg.weatherCityCode);
  root.field("weatherInterval", cfg.weatherRefreshMinutes);
  root.field("rotation", cfg.displayRotation);
  root.field("ntpServer", cfg.timeServer);
  root.field("sound", cfg.touchSoundEnabled);
  root.field("alarmEnabled", cfg.alarmEnabled);
  root.field("alarmTime", cfg.alarmTime);
  root.field("snooze", cfg.alarmSnoozeMinutes);
  root.field("albumInterval", cfg.photoIntervalSeconds);
  root.field("language", cfg.displayLanguage);
  root.field("clockColor", cfg.clockHourColor);
  root.field("alarmDays", cfg.alarmWeekdayMask);
  root.field("alarmTune", cfg.alarmMelody);
  root.field("minuteColor", cfg.clockMinuteColor);
  root.field("adminLanguage", cfg.adminLanguage);
  root.field("albumFixed", cfg.photoFixedMode);
  root.field("showSeconds", cfg.clockSecondsEnabled);
  root.field("nasBackground", m.monitorBackground);
  root.field("cardBackground", m.monitorCardBackground);
  root.finish();
}
class ConfigStringSink : public Print {
public:
  String value;
  ConfigStringSink() { value.reserve(2048); }
  size_t write(uint8_t byte) override { value += char(byte); return 1; }
};
String publicConfig(int selected = -1) {
  ConfigStringSink sink;
  emitPublicConfig(sink, selected);
  return std::move(sink.value);
}
class ConfigWebSink : public Print {
  // Static workspace is safe: all route handlers execute on the single main loop.
  static constexpr unsigned Capacity = sizeof(RuntimeWorkspace::bytes);
  unsigned used = 0;
  char *buffer;
public:
  ConfigWebSink() { buffer = reinterpret_cast<char *>(RuntimeWorkspace::bytes); }
  void flush() { if (used) web.sendContent(buffer, used); used = 0; }
  size_t write(uint8_t byte) override {
    buffer[used++] = byte; if (used == Capacity) flush(); return 1;
  }
};
class ConfigCountSink : public Print {
public:
  size_t count = 0;
  size_t write(uint8_t) override { ++count; return 1; }
};
void home() {
  // Provisioning is explicit and independent of a previous management cookie.
  if (ap && web.uri() != "/manage") { entryPage(true); return; }
  if (!sessionValid()) {
    entryPage(ap && web.uri() != "/manage");
    return;
  }
  if (cfg.adminDefault) {
    entryPage(false);
    return;
  }
  secureHeaders();
  unsigned edit = activeMachine;
  if (web.hasArg("device") && !AccessPolicy::decimal(web.arg("device").c_str(), machineCount - 1, edit)) { web.send(400, "text/plain", "Invalid device"); return; }
  // Generate dynamic configuration only at its placeholder. Holding a 2 KiB
  // JSON String throughout HTML/TCP sending starves the AP+STA network stack.
  ConfigCountSink counter;
  emitPublicConfig(counter, edit);
  size_t length = 0;
  for (const auto &stored : webSegments) {
    WebSegment segment;
    memcpy_P(&segment, &stored, sizeof(segment));
    length += segment.length;
    if (segment.key == 1) length += csrf.length();
    else if (segment.key == 2) length += counter.count;
  }
  // A known length avoids three synchronous writes per HTTP chunk while still
  // streaming the template and JSON through the bounded shared workspace.
  web.setContentLength(length);
  web.sendHeader("Connection", "close");
  web.send(200, "text/html; charset=utf-8", "");
  for (unsigned i = 0; i < sizeof(webSegments) / sizeof(WebSegment); i++) {
    WebSegment segment;
    memcpy_P(&segment, webSegments + i, sizeof(segment));
    if (segment.length)
      sendFlashContent(managementHTML + segment.offset, segment.length);
    if (segment.key == 1) web.sendContent(csrf);
    else if (segment.key == 2) {
      // Reclaim acknowledged template packets before building dynamic JSON.
      // Otherwise TCP and String escaping can compete for the same heap block.
      web.client().flush();
      ConfigWebSink sink;
      emitPublicConfig(sink, edit);
      sink.flush();
    }
    yield();
  }
  web.sendContent("");
}
