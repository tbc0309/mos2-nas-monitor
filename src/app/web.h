#pragma once
// Authenticated management API and configuration validation.
void startAP() {
  if (ap)
    return;
  weatherClient.cancel();
  ap = true;
  qrPage = false;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(apName.c_str(), apPass.c_str());
  dns.start(53, "*", WiFi.softAPIP());
  draw();
}
String publicConfig() {
  return "{\"ssid\":" + jsonString(cfg.ssid) + ",\"nas\":" + jsonString(cfg.nas) +
         ",\"mode\":" + jsonString("snmp") + ",\"port\":" + String(cfg.port) +
         ",\"storage\":" + jsonString(cfg.storage) + ",\"iface\":" + jsonString(cfg.iface) +
         ",\"home\":" + String(cfg.home) + ",\"interval\":" + String(cfg.interval) +
         ",\"brightness\":" + String(cfg.brightness) +
         ",\"weatherEnabled\":" + String(cfg.weatherEnabled) +
         ",\"cityCode\":" + jsonString(cfg.cityCode) +
         ",\"weatherInterval\":" + String(cfg.weatherInterval) +
         ",\"rotation\":" + String(cfg.rotation) + ",\"ntpServer\":" + jsonString(cfg.ntpServer) +
         ",\"sound\":" + String(cfg.sound) + ",\"alarmEnabled\":" + String(cfg.alarmEnabled) +
         ",\"alarmTime\":" + jsonString(cfg.alarmTime) + ",\"snooze\":" + String(cfg.snooze) +
         ",\"albumInterval\":" + String(cfg.albumInterval) +
         ",\"language\":" + jsonString(cfg.language) +
         ",\"clockColor\":" + jsonString(cfg.clockColor) +
         ",\"alarmDays\":" + String(cfg.alarmDays) + ",\"alarmTune\":" + String(cfg.alarmTune) +
         ",\"minuteColor\":" + jsonString(cfg.minuteColor) +
         ",\"adminLanguage\":" + jsonString(cfg.adminLanguage) +
         ",\"albumFixed\":" + String(cfg.albumFixed) +
         ",\"showSeconds\":" + String(cfg.showSeconds) +
         ",\"nasBackground\":" + jsonString(cfg.nasBackground) + "}";
}
void home() {
  if (!sessionValid()) {
    entryPage(ap && web.uri() != "/manage");
    return;
  }
  web.sendHeader("Cache-Control", "no-store");
  String values[5];
  values[1] = csrf;
  values[2] = publicConfig();
  values[3] = "";
  size_t length = 0;
  for (unsigned i = 0; i < sizeof(webSegments) / sizeof(WebSegment); i++) {
    WebSegment segment;
    memcpy_P(&segment, webSegments + i, sizeof(segment));
    length += segment.length + values[segment.key].length();
  }
  web.setContentLength(length);
  web.send(200, "text/html; charset=utf-8", "");
  for (unsigned i = 0; i < sizeof(webSegments) / sizeof(WebSegment); i++) {
    WebSegment segment;
    memcpy_P(&segment, webSegments + i, sizeof(segment));
    if (segment.length)
      web.sendContent_P(managementHTML + segment.offset, segment.length);
    if (values[segment.key].length())
      web.sendContent(values[segment.key]);
    yield();
  }
}
String weatherStatus() {
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
         ",\"age\":" + String(weather.valid ? (millis() - weather.fetched) / 1000 : 0) + "}";
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
  String j =
      "{\"mode\":" + jsonString("snmp") + ",\"page\":" + String(page) +
      ",\"online\":" + String(metric.online ? "true" : "false") + ",\"cpu\":" + String(metric.cpu) +
      ",\"memoryTotal\":" + String(metric.total * 1024) +
      ",\"memoryAvail\":" + String(metric.avail * 1024) + ",\"memory\":" + String(metric.memory) +
      ",\"storage\":" + String(metric.storage) + ",\"temperature\":" + String(metric.temp) +
      ",\"rx\":" + String(metric.rx) + ",\"tx\":" + String(metric.tx) +
      ",\"volume\":" + jsonString(metric.volume) + ",\"interface\":" + jsonString(metric.iface) +
      ",\"storageIndex\":" + jsonString(storageIndex) +
      ",\"interfaceIndex\":" + jsonString(interfaceIndex) +
      ",\"interfaces\":" + interfacesStatus() + ",\"model\":" + jsonString(metric.model) +
      ",\"ip\":" + jsonString(WiFi.localIP().toString()) +
      ",\"freeHeap\":" + String(ESP.getFreeHeap()) + ",\"clock\":" + jsonString(screen.clock) +
      ",\"date\":" + jsonString(screen.date) + ",\"weather\":" + weatherStatus() +
      ",\"disks\":" + diskStatus() + ",\"diskUpdated\":" + String(nasExtra.diskUpdated) +
      ",\"volumeUpdated\":" + String(nasExtra.volumeUpdated) + ",\"volumes\":" + volumeStatus() +
      ",\"alarm\":" + alarmStatus() + ",\"albumCount\":" + String(albumCount) +
      ",\"albumDraws\":" + String(albumDraws) + ",\"albumFixed\":" + String(cfg.albumFixed) +
      ",\"albumSlot\":" + String(albumCount ? albumSlots[albumPosition] : -1) +
      ",\"diskGroup\":" + String(screen.diskGroup) +
      ",\"volumeGroup\":" + String(screen.volumeGroup) +
      ",\"resetPending\":" + String(resetConfirm.pending ? "true" : "false") +
      ",\"language\":" + jsonString(cfg.language) + "}";
  web.send(200, "application/json", j);
}
bool cityOK(const String &s) {
  if (s.length() != 9)
    return false;
  for (char c : s)
    if (c < '0' || c > '9')
      return false;
  return s.startsWith("101") || s.startsWith("102");
}
bool ntpOK(const String &s) {
  if (!s.length() || s.length() > 63)
    return false;
  for (char c : s)
    if (!isalnum(c) && c != '.' && c != '-')
      return false;
  return true;
}
bool indexOK(String s) {
  if (!s.length())
    return true;
  for (char c : s)
    if (c < '0' || c > '9')
      return false;
  return s.toInt() > 0;
}
bool alarmTimeOK(const String &s) {
  return s.length() == 5 && s[2] == ':' && isdigit(s[0]) && isdigit(s[1]) && isdigit(s[3]) &&
         isdigit(s[4]) && s.substring(0, 2).toInt() < 24 && s.substring(3, 5).toInt() < 60;
}
void settings() {
  if (!authorized())
    return;
  if (web.arg("token") != csrf) {
    web.send(403, "text/plain", "Reload the settings page.");
    return;
  }
  if (web.hasArg("nasBackground")) {
    String bg = web.arg("nasBackground");
    bool ok = bg.length() == 7 && bg[0] == '#';
    for (unsigned i = 1; i < bg.length(); i++)
      if (!isxdigit(bg[i]))
        ok = false;
    if (!ok) {
      web.send(400, "text/plain", "Invalid background color");
      return;
    }
  }
  IPAddress ip;
  String host = web.arg("nas"), ss = web.arg("ssid"), st = web.arg("storage"),
         in = web.arg("iface");
  unsigned port = web.arg("port").toInt(), iv = web.arg("interval").toInt(),
           bl = web.arg("brightness").toInt();
  if (!ip.fromString(host) || !ss.length() || ss.length() > 32 || port < 1 || port > 65535 ||
      iv < 3 || iv > 60 || bl < 5 || bl > 100 || !indexOK(st) || !indexOK(in) ||
      web.arg("pass").length() > 63 || web.arg("community").length() > 64 ||
      web.arg("admin").length() > 64 || web.arg("home").toInt() < 0 ||
      web.arg("home").toInt() > 9 || web.arg("home").toInt() == 3 ||
      (web.arg("home") == "5" && web.arg("weatherEnabled") == "0") ||
      !cityOK(web.arg("cityCode")) || !ntpOK(web.arg("ntpServer")) ||
      (web.arg("weatherEnabled") != "0" && web.arg("weatherEnabled") != "1") ||
      web.arg("weatherInterval").toInt() < 5 || web.arg("weatherInterval").toInt() > 60 ||
      web.arg("rotation").toInt() < 0 || web.arg("rotation").toInt() > 3 ||
      (web.arg("sound") != "0" && web.arg("sound") != "1") ||
      (web.arg("alarmEnabled") != "0" && web.arg("alarmEnabled") != "1") ||
      !alarmTimeOK(web.arg("alarmTime")) || web.arg("snooze").toInt() < 1 ||
      web.arg("snooze").toInt() > 30 || web.arg("albumInterval").toInt() < 3 ||
      web.arg("albumInterval").toInt() > 300) {
    web.send(400, "text/plain; charset=utf-8",
             WebMessages::text("Invalid settings. Check addresses and values.",
                               cfg.adminLanguage == "en"));
    return;
  }
  const Config previousConfig = cfg;
  if (web.hasArg("language")) {
    String lang = web.arg("language"), color = web.arg("clockColor");
    bool valid = (lang == "zh" || lang == "en") && color.length() == 7 && color[0] == '#' &&
                 web.arg("alarmDays").toInt() > 0 && web.arg("alarmDays").toInt() <= 127 &&
                 web.arg("alarmTune").toInt() >= 0 && web.arg("alarmTune").toInt() <= 3;
    for (unsigned i = 1; i < color.length(); i++)
      if (!isxdigit(color[i]))
        valid = false;
    if (!valid) {
      web.send(400, "text/plain", "Invalid display/alarm settings");
      return;
    }
    String minute = web.hasArg("minuteColor") ? web.arg("minuteColor") : color;
    bool colorValid = minute.length() == 7 && minute[0] == '#';
    for (unsigned i = 1; i < minute.length(); i++)
      if (!isxdigit(minute[i]))
        colorValid = false;
    if (!colorValid || (web.hasArg("albumFixed") && web.arg("albumFixed") != "0" &&
                        web.arg("albumFixed") != "1")) {
      web.send(400, "text/plain", "Invalid display settings");
      return;
    }
    if (web.hasArg("showSeconds")) {
      if (web.arg("showSeconds") != "0" && web.arg("showSeconds") != "1") {
        web.send(400, "text/plain", "Invalid seconds setting");
        return;
      }
      cfg.showSeconds = web.arg("showSeconds").toInt();
    }
    cfg.minuteColor = minute;
    if (web.hasArg("albumFixed"))
      cfg.albumFixed = web.arg("albumFixed").toInt();
    cfg.language = lang;
    cfg.clockColor = color;
    cfg.alarmDays = web.arg("alarmDays").toInt();
    cfg.alarmTune = web.arg("alarmTune").toInt();
  }
  if (web.hasArg("nasBackground"))
    cfg.nasBackground = web.arg("nasBackground");
  cfg.ssid = ss;
  if (web.arg("pass").length())
    cfg.pass = web.arg("pass");
  if (web.arg("community").length())
    cfg.community = web.arg("community");
  if (web.arg("admin").length())
    cfg.admin = web.arg("admin");
  cfg.sound = web.arg("sound").toInt();
  cfg.alarmEnabled = web.arg("alarmEnabled").toInt();
  if (cfg.alarmTime != web.arg("alarmTime")) {
    LittleFS.remove("/alarm-day");
    alarmClock.lastDay = 0;
  }
  cfg.alarmTime = web.arg("alarmTime");
  cfg.snooze = web.arg("snooze").toInt();
  cfg.albumInterval = web.arg("albumInterval").toInt();
  cfg.weatherEnabled = web.arg("weatherEnabled").toInt();
  cfg.cityCode = web.arg("cityCode");
  cfg.weatherInterval = web.arg("weatherInterval").toInt();
  cfg.rotation = web.arg("rotation").toInt();
  cfg.ntpServer = web.arg("ntpServer");
  cfg.home = web.arg("home").toInt();
  cfg.nas = host;
  cfg.storage = st;
  cfg.iface = in;
  cfg.port = port;
  cfg.interval = iv;
  cfg.brightness = bl;
  if (!save()) {
    cfg = previousConfig;
    web.send(500, "text/plain", "Configuration write failed");
    return;
  }
  web.send(200, "text/plain; charset=utf-8",
           WebMessages::text("Saved. Restarting device.", cfg.adminLanguage == "en"));
  reboot = true;
  rebootAt = millis();
}
