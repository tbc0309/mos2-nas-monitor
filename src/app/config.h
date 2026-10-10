#pragma once
String escapeHtml(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
}
String jsonString(String s) {
  String r = "\"";
  for (unsigned i = 0; i < s.length(); i++) {
    uint8_t c = s[i];
    if (c == '"' || c == '\\') {
      r += '\\';
      r += char(c);
    } else if (c == '<')
      r += "\\u003c";
    else if (c >= 32)
      r += char(c);
  }
  return r + '"';
}
// Persistent schema 1 is independent of the firmware release number.
// Write to a temporary file and atomically rename only after a complete flush.
String line(File &f) {
  String s = f.readStringUntil('\n');
  s.trim();
  return s;
}
bool save() {
  StaticJsonDocument<2048> doc;
  doc["schema"] = 1;
  doc["ssid"] = cfg.ssid;
  doc["pass"] = cfg.pass;
  doc["nas"] = cfg.nas;
  doc["community"] = cfg.community;
  doc["admin"] = cfg.admin;
  doc["storage"] = cfg.storage;
  doc["iface"] = cfg.iface;
  doc["cityCode"] = cfg.cityCode;
  doc["ntpServer"] = cfg.ntpServer;
  doc["weatherEnabled"] = cfg.weatherEnabled;
  doc["weatherInterval"] = cfg.weatherInterval;
  doc["rotation"] = cfg.rotation;
  doc["sound"] = cfg.sound;
  doc["alarmTime"] = cfg.alarmTime;
  doc["alarmEnabled"] = cfg.alarmEnabled;
  doc["snooze"] = cfg.snooze;
  doc["albumInterval"] = cfg.albumInterval;
  doc["language"] = cfg.language;
  doc["clockColor"] = cfg.clockColor;
  doc["nasBackground"] = cfg.nasBackground;
  doc["minuteColor"] = cfg.minuteColor;
  doc["adminLanguage"] = cfg.adminLanguage;
  doc["albumFixed"] = cfg.albumFixed;
  doc["showSeconds"] = cfg.showSeconds;
  doc["alarmDays"] = cfg.alarmDays;
  doc["alarmTune"] = cfg.alarmTune;
  doc["home"] = cfg.home;
  doc["port"] = cfg.port;
  doc["interval"] = cfg.interval;
  doc["brightness"] = cfg.brightness;
  if (doc.overflowed())
    return false;
  File f = LittleFS.open("/config.tmp", "w");
  if (!f)
    return false;
  size_t expected = measureJson(doc), written = serializeJson(doc, f);
  f.flush();
  f.close();
  if (written != expected) {
    LittleFS.remove("/config.tmp");
    return false;
  }
  if (!LittleFS.rename("/config.tmp", "/config.json")) {
    LittleFS.remove("/config.tmp");
    return false;
  }
  return true;
}
bool colorOK(const String &value) {
  if (value.length() != 7 || value[0] != '#')
    return false;
  for (unsigned i = 1; i < 7; i++)
    if (!isxdigit(value[i]))
      return false;
  return true;
}
extern "C" uint32_t _FS_start, _FS_end;
bool blankFilesystem() {
  const uint32_t begin = uintptr_t(&_FS_start) - 0x40200000u;
  const uint32_t end = uintptr_t(&_FS_end) - 0x40200000u;
  if (end > ESP.getFlashChipRealSize())
    return false;
  return Storage::isErased(
      begin, end,
      [](uint32_t address, uint32_t *words, size_t size) {
        return ESP.flashRead(address, words, size);
      },
      []() { yield(); });
}
void load() {
  LittleFS.setConfig(LittleFSConfig().setAutoFormat(false));
  if (!LittleFS.begin()) {
    if (!blankFilesystem() || !LittleFS.format() || !LittleFS.begin()) {
      Serial.println("Filesystem mount failed; data preserved");
      return;
    }
    Serial.println("Blank filesystem initialized");
  }
  File f = LittleFS.open("/config.json", "r");
  StaticJsonDocument<2048> doc;
  if (f && !deserializeJson(doc, f) && doc["schema"] == 1) {
    if (doc["ssid"].is<const char *>())
      cfg.ssid = doc["ssid"].as<String>();
    if (doc["pass"].is<const char *>())
      cfg.pass = doc["pass"].as<String>();
    if (doc["nas"].is<const char *>())
      cfg.nas = doc["nas"].as<String>();
    if (doc["community"].is<const char *>())
      cfg.community = doc["community"].as<String>();
    if (doc["admin"].is<const char *>())
      cfg.admin = doc["admin"].as<String>();
    if (doc["storage"].is<const char *>())
      cfg.storage = doc["storage"].as<String>();
    if (doc["iface"].is<const char *>())
      cfg.iface = doc["iface"].as<String>();
    if (doc["cityCode"].is<const char *>())
      cfg.cityCode = doc["cityCode"].as<String>();
    if (doc["ntpServer"].is<const char *>())
      cfg.ntpServer = doc["ntpServer"].as<String>();
    if (doc["weatherEnabled"].is<unsigned>())
      cfg.weatherEnabled = doc["weatherEnabled"].as<unsigned>();
    if (doc["weatherInterval"].is<unsigned>())
      cfg.weatherInterval = doc["weatherInterval"].as<unsigned>();
    if (doc["rotation"].is<unsigned>())
      cfg.rotation = doc["rotation"].as<unsigned>();
    if (doc["sound"].is<unsigned>())
      cfg.sound = doc["sound"].as<unsigned>();
    if (doc["alarmTime"].is<const char *>())
      cfg.alarmTime = doc["alarmTime"].as<String>();
    if (doc["alarmEnabled"].is<unsigned>())
      cfg.alarmEnabled = doc["alarmEnabled"].as<unsigned>();
    if (doc["snooze"].is<unsigned>())
      cfg.snooze = doc["snooze"].as<unsigned>();
    if (doc["albumInterval"].is<unsigned>())
      cfg.albumInterval = doc["albumInterval"].as<unsigned>();
    if (doc["language"].is<const char *>())
      cfg.language = doc["language"].as<String>();
    if (doc["clockColor"].is<const char *>())
      cfg.clockColor = doc["clockColor"].as<String>();
    if (doc["nasBackground"].is<const char *>())
      cfg.nasBackground = doc["nasBackground"].as<String>();
    if (doc["minuteColor"].is<const char *>())
      cfg.minuteColor = doc["minuteColor"].as<String>();
    if (doc["adminLanguage"].is<const char *>())
      cfg.adminLanguage = doc["adminLanguage"].as<String>();
    if (doc["albumFixed"].is<unsigned>())
      cfg.albumFixed = doc["albumFixed"].as<unsigned>();
    if (doc["showSeconds"].is<unsigned>())
      cfg.showSeconds = doc["showSeconds"].as<unsigned>();
    if (doc["alarmDays"].is<unsigned>())
      cfg.alarmDays = doc["alarmDays"].as<unsigned>();
    if (doc["alarmTune"].is<unsigned>())
      cfg.alarmTune = doc["alarmTune"].as<unsigned>();
    if (doc["home"].is<unsigned>())
      cfg.home = doc["home"].as<unsigned>();
    if (doc["port"].is<unsigned>())
      cfg.port = doc["port"].as<unsigned>();
    if (doc["interval"].is<unsigned>())
      cfg.interval = doc["interval"].as<unsigned>();
    if (doc["brightness"].is<unsigned>())
      cfg.brightness = doc["brightness"].as<unsigned>();
  }
  f.close();
  if (!cfg.admin.length())
    cfg.admin = "12345678";
  cfg.port = constrain(cfg.port, 1u, 65535u);
  cfg.weatherEnabled = !!cfg.weatherEnabled;
  cfg.weatherInterval = constrain(cfg.weatherInterval, 5u, 60u);
  cfg.rotation = constrain(cfg.rotation, 0u, 3u);
  if (cfg.cityCode.length() != 9)
    cfg.cityCode = "101030100";
  if (!cfg.ntpServer.length())
    cfg.ntpServer = "ntp.aliyun.com";
  cfg.sound = !!cfg.sound;
  cfg.alarmEnabled = !!cfg.alarmEnabled;
  cfg.snooze = constrain(cfg.snooze, 1u, 30u);
  cfg.albumInterval = constrain(cfg.albumInterval, 3u, 300u);
  cfg.home = constrain(cfg.home, 0u, 9u);
  if (cfg.home == 3)
    cfg.home = 6;
  if (cfg.home == 5 && !cfg.weatherEnabled)
    cfg.home = 6;
  cfg.interval = constrain(cfg.interval, 3u, 60u);
  cfg.brightness = constrain(cfg.brightness, 5u, 100u);
  if (cfg.language != "en")
    cfg.language = "zh";
  if (cfg.adminLanguage != "en")
    cfg.adminLanguage = "zh";
  if (!colorOK(cfg.clockColor))
    cfg.clockColor = "#ffffff";
  if (!colorOK(cfg.minuteColor))
    cfg.minuteColor = "#ffa500";
  if (!colorOK(cfg.nasBackground))
    cfg.nasBackground = "#21455c";
  cfg.alarmDays = constrain(cfg.alarmDays, 1u, 127u);
  cfg.alarmTune = constrain(cfg.alarmTune, 0u, 3u);
  cfg.albumFixed = !!cfg.albumFixed;
  cfg.showSeconds = !!cfg.showSeconds;
  // Normalize in RAM; only explicit saves should write configuration to flash.
}
