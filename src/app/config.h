#pragma once
#include "json_fields.h"
String escapeHtml(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
}
String jsonString(const String &s) {
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
// Persistent schema 3 is independent of the firmware release number.
// Write to a temporary file and atomically rename only after a complete flush.
String line(File &f) {
  String s = f.readStringUntil('\n');
  s.trim();
  return s;
}
bool save() {
  Serial.printf("SAVE begin heap=%u block=%u stack=%u\n", ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(), ESP.getFreeContStack());
  captureMachine();
  const bool defaultPassword = cfg.adminPasswordVerifier.startsWith("p2$") ? cfg.adminDefault : cfg.adminPasswordVerifier == "12345678";
  const String verifier = cfg.adminPasswordVerifier.startsWith("p2$") ? cfg.adminPasswordVerifier : Credentials::passwordHash(cfg.adminPasswordVerifier);
  File f = LittleFS.open("/config.tmp", "w");
  if (!f)
    return false;
  uint8_t keys[64];
  if (!Credentials::key(keys, true)) { f.close(); LittleFS.remove("/config.tmp"); return false; }
  // HTTP handlers already hold form data and rollback state on the 4 KiB
  // continuation stack. Keep the crypto contexts off that nested stack.
  // The single main-loop writer uses reusable storage: browser TCP bursts can
  // fragment the heap even when total free memory would fit the crypto context.
  static_assert(sizeof(Credentials::Writer) <= sizeof(RuntimeWorkspace::bytes), "Crypto workspace capacity");
  Credentials::Writer *writer = new (RuntimeWorkspace::bytes) Credentials::Writer(f, keys);
  JsonFields root(*writer);
  root.field("schema", 3);
  root.field("ssid", cfg.wifiSsid);
  root.field("pass", cfg.wifiPassword);
  root.field("admin", verifier);
  root.field("adminDefault", defaultPassword);
  root.field("cityCode", cfg.weatherCityCode);
  root.field("ntpServer", cfg.timeServer);
  root.field("weatherEnabled", cfg.weatherEnabled);
  root.field("weatherInterval", cfg.weatherRefreshMinutes);
  root.field("rotation", cfg.displayRotation);
  root.field("sound", cfg.touchSoundEnabled);
  root.field("alarmTime", cfg.alarmTime);
  root.field("alarmEnabled", cfg.alarmEnabled);
  root.field("snooze", cfg.alarmSnoozeMinutes);
  root.field("albumInterval", cfg.photoIntervalSeconds);
  root.field("language", cfg.displayLanguage);
  root.field("clockColor", cfg.clockHourColor);
  root.field("minuteColor", cfg.clockMinuteColor);
  root.field("adminLanguage", cfg.adminLanguage);
  root.field("albumFixed", cfg.photoFixedMode);
  root.field("showSeconds", cfg.clockSecondsEnabled);
  root.field("alarmDays", cfg.alarmWeekdayMask);
  root.field("alarmTune", cfg.alarmMelody);
  root.field("home", cfg.startupPage);
  root.field("brightness", cfg.displayBrightnessPercent);
  root.field("activeMachine", activeMachine);
  root.key("machines"); writer->write('[');
  for (unsigned i = 0; i < machineCount; ++i) {
    if (i) writer->write(',');
    const auto &m = machines[i];
    JsonFields profile(*writer);
    profile.field("nasBackground", m.monitorBackground);
    profile.field("cardBackground", m.monitorCardBackground);
    profile.field("name", m.name);
    profile.field("nas", m.snmpHost);
    profile.field("community", m.snmpCommunity);
    profile.field("snmpVersion", m.snmpVersion);
    profile.field("snmpUser", m.snmpUsername);
    profile.field("snmpPassword", m.snmpAuthPassword);
    profile.field("snmpAuth", m.snmpAuthProtocol);
    profile.field("snmpPrivacy", m.snmpPrivacyProtocol);
    profile.field("snmpPrivacyPassword", m.snmpPrivacyPassword);
    profile.field("iface", m.networkInterfaceIndex);
    profile.field("storage", m.storageVolumeIndex);
    profile.field("port", m.snmpPort);
    profile.field("interval", m.snmpPollIntervalSeconds);
    profile.finish();
  }
  writer->write(']'); root.finish();
  const bool encrypted = writer->finish();
  writer->~Writer();
  f.flush();
  f.close();
  if (!encrypted) {
    LittleFS.remove("/config.tmp");
    return false;
  }
  if (!LittleFS.rename("/config.tmp", "/config.json")) {
    LittleFS.remove("/config.tmp");
    return false;
  }
  cfg.adminPasswordVerifier = verifier; cfg.adminDefault = defaultPassword;
  Serial.printf("SAVE complete heap=%u stack=%u\n", ESP.getFreeHeap(), ESP.getFreeContStack());
  machineSelectionDirty = false;
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
  // Interrupted writes are uncommitted, never a second source of credentials.
  LittleFS.remove("/config.tmp");
  LittleFS.remove("/key.tmp");
  LittleFS.remove("/incoming.jpg");
  DynamicJsonDocument doc(JSON_OBJECT_SIZE(48) + JSON_ARRAY_SIZE(MaxMachines) + MaxMachines * JSON_OBJECT_SIZE(20));
  String plaintext;
  const bool validDocument = Credentials::read(f, plaintext) && !deserializeJson(doc, plaintext.begin()) &&
                             doc["schema"] == 3 && String(doc["admin"] | "").startsWith("p2$");
  if (validDocument) {
    cfg.adminDefault = doc["adminDefault"] | false;
    if (doc["ssid"].is<const char *>())
      cfg.wifiSsid = doc["ssid"].as<String>();
    if (doc["pass"].is<const char *>())
      cfg.wifiPassword = doc["pass"].as<String>();
    if (doc["admin"].is<const char *>())
      cfg.adminPasswordVerifier = doc["admin"].as<String>();
    if (doc["cityCode"].is<const char *>())
      cfg.weatherCityCode = doc["cityCode"].as<String>();
    if (doc["ntpServer"].is<const char *>())
      cfg.timeServer = doc["ntpServer"].as<String>();
    if (doc["weatherEnabled"].is<unsigned>())
      cfg.weatherEnabled = doc["weatherEnabled"].as<unsigned>();
    if (doc["weatherInterval"].is<unsigned>())
      cfg.weatherRefreshMinutes = doc["weatherInterval"].as<unsigned>();
    if (doc["rotation"].is<unsigned>())
      cfg.displayRotation = doc["rotation"].as<unsigned>();
    if (doc["sound"].is<unsigned>())
      cfg.touchSoundEnabled = doc["sound"].as<unsigned>();
    if (doc["alarmTime"].is<const char *>())
      cfg.alarmTime = doc["alarmTime"].as<String>();
    if (doc["alarmEnabled"].is<unsigned>())
      cfg.alarmEnabled = doc["alarmEnabled"].as<unsigned>();
    if (doc["snooze"].is<unsigned>())
      cfg.alarmSnoozeMinutes = doc["snooze"].as<unsigned>();
    if (doc["albumInterval"].is<unsigned>())
      cfg.photoIntervalSeconds = doc["albumInterval"].as<unsigned>();
    if (doc["language"].is<const char *>())
      cfg.displayLanguage = doc["language"].as<String>();
    if (doc["clockColor"].is<const char *>())
      cfg.clockHourColor = doc["clockColor"].as<String>();
    cfg.monitorCardBackground = doc["cardBackground"] | "#103040";
    if (doc["minuteColor"].is<const char *>())
      cfg.clockMinuteColor = doc["minuteColor"].as<String>();
    if (doc["adminLanguage"].is<const char *>())
      cfg.adminLanguage = doc["adminLanguage"].as<String>();
    if (doc["albumFixed"].is<unsigned>())
      cfg.photoFixedMode = doc["albumFixed"].as<unsigned>();
    if (doc["showSeconds"].is<unsigned>())
      cfg.clockSecondsEnabled = doc["showSeconds"].as<unsigned>();
    if (doc["alarmDays"].is<unsigned>())
      cfg.alarmWeekdayMask = doc["alarmDays"].as<unsigned>();
    if (doc["alarmTune"].is<unsigned>())
      cfg.alarmMelody = doc["alarmTune"].as<unsigned>();
    if (doc["home"].is<unsigned>())
      cfg.startupPage = doc["home"].as<unsigned>();
    if (doc["brightness"].is<unsigned>())
      cfg.displayBrightnessPercent = doc["brightness"].as<unsigned>();
  }
  if (validDocument) readMachines(doc);
  else captureMachine();
  f.close();
  if (!cfg.adminPasswordVerifier.length())
    cfg.adminPasswordVerifier = "12345678";
  if (cfg.snmpVersion != "3")
    cfg.snmpVersion = "2c";
  if (cfg.snmpAuthProtocol != "SHA") cfg.snmpAuthProtocol = "MD5";
  if (cfg.snmpPrivacyProtocol != "DES" && cfg.snmpPrivacyProtocol != "AES")
    cfg.snmpPrivacyProtocol = "none";
  cfg.snmpPort = constrain(cfg.snmpPort, 1u, 65535u);
  cfg.weatherEnabled = !!cfg.weatherEnabled;
  cfg.weatherRefreshMinutes = constrain(cfg.weatherRefreshMinutes, 10u, 120u);
  cfg.displayRotation = constrain(cfg.displayRotation, 0u, 3u);
  if (cfg.weatherCityCode.length() != 9)
    cfg.weatherCityCode = "101030100";
  if (!cfg.timeServer.length())
    cfg.timeServer = "ntp.aliyun.com";
  cfg.touchSoundEnabled = !!cfg.touchSoundEnabled;
  cfg.alarmEnabled = !!cfg.alarmEnabled;
  cfg.alarmSnoozeMinutes = constrain(cfg.alarmSnoozeMinutes, 1u, 30u);
  cfg.photoIntervalSeconds = constrain(cfg.photoIntervalSeconds, 3u, 300u);
  cfg.startupPage = constrain(cfg.startupPage, 0u, 9u);
  if (cfg.startupPage == 3)
    cfg.startupPage = 6;
  if (cfg.startupPage == 5 && !cfg.weatherEnabled)
    cfg.startupPage = 6;
  cfg.snmpPollIntervalSeconds = constrain(cfg.snmpPollIntervalSeconds, 3u, 60u);
  cfg.displayBrightnessPercent = constrain(cfg.displayBrightnessPercent, 5u, 100u);
  if (cfg.displayLanguage != "en")
    cfg.displayLanguage = "zh";
  if (cfg.adminLanguage != "en")
    cfg.adminLanguage = "zh";
  if (!colorOK(cfg.clockHourColor))
    cfg.clockHourColor = "#ffffff";
  if (!colorOK(cfg.clockMinuteColor))
    cfg.clockMinuteColor = "#ffa500";
  if (!colorOK(cfg.monitorCardBackground)) cfg.monitorCardBackground = "#103040";
  if (!colorOK(cfg.monitorBackground))
    cfg.monitorBackground = "#21455c";
  cfg.alarmWeekdayMask = constrain(cfg.alarmWeekdayMask, 1u, 127u);
  cfg.alarmMelody = constrain(cfg.alarmMelody, 0u, 3u);
  cfg.photoFixedMode = !!cfg.photoFixedMode;
  cfg.clockSecondsEnabled = !!cfg.clockSecondsEnabled;
  // Normalize in RAM; only explicit saves should write configuration to flash.
}
