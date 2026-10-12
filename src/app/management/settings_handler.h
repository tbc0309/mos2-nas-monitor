#pragma once
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
    web.send(403, "text/plain; charset=utf-8", WebMessages::text("Reload the settings page.", cfg.adminLanguage == "en"));
    return;
  }
  if (web.hasArg("cardBackground") && !colorOK(web.arg("cardBackground"))) { web.send(400, "text/plain", "Invalid color"); return; }
  if (web.hasArg("nasBackground")) {
    String bg = web.arg("nasBackground");
    bool ok = bg.length() == 7 && bg[0] == '#';
    for (unsigned i = 1; i < bg.length(); i++)
      if (!isxdigit(bg[i]))
        ok = false;
    if (!ok) {
      web.send(400, "text/plain; charset=utf-8", WebMessages::text("Invalid background color", cfg.adminLanguage == "en"));
      return;
    }
  }
  unsigned edit = activeMachine;
  if (web.hasArg("machineIndex") && !AccessPolicy::decimal(web.arg("machineIndex").c_str(), machineCount - 1, edit)) {
    web.send(400, "text/plain", "Invalid device"); return;
  }
  if (cityOK(web.arg("cityCode")) && !WeatherValidation::ready(web.arg("cityCode"))) {
    web.send(409, "text/plain; charset=utf-8",
             WebMessages::text("Verify the city code before saving.", cfg.adminLanguage == "en"));
    return;
  }
  const auto &editing = machines[edit];
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
      web.arg("weatherInterval").toInt() < 10 || web.arg("weatherInterval").toInt() > 120 ||
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
  if (web.arg("admin").length() &&
      (web.arg("admin").length() < 8 || web.arg("admin") == "12345678")) {
    web.send(400, "text/plain; charset=utf-8",
             WebMessages::text("Choose a new password of at least 8 bytes.", cfg.adminLanguage == "en"));
    return;
  }
  String version = web.hasArg("snmpVersion") ? web.arg("snmpVersion") : editing.snmpVersion;
  String user = web.hasArg("snmpUser") ? web.arg("snmpUser") : editing.snmpUsername;
  String authPassword =
      web.arg("snmpPassword").length() ? web.arg("snmpPassword") : editing.snmpAuthPassword;
  String authProtocol = web.hasArg("snmpAuth") ? web.arg("snmpAuth") : editing.snmpAuthProtocol;
  if ((version != "2c" && version != "3") || (authProtocol != "MD5" && authProtocol != "SHA") || user.length() > 32 ||
      authPassword.length() > 64 ||
      (version == "3" && (!user.length() || authPassword.length() < 8))) {
    web.send(400, "text/plain; charset=utf-8",
             WebMessages::text(
                 "SNMP v3 requires a username and an authentication password (8–64 bytes).",
                 cfg.adminLanguage == "en"));
    return;
  }
  String privacy = web.hasArg("snmpPrivacy") ? web.arg("snmpPrivacy") : editing.snmpPrivacyProtocol;
  String privatePassword = web.arg("snmpPrivacyPassword").length() ? web.arg("snmpPrivacyPassword")
                                                                   : editing.snmpPrivacyPassword;
  if ((privacy != "none" && privacy != "DES" && privacy != "AES") || privatePassword.length() > 64 ||
      (version == "3" && privacy != "none" && privatePassword.length() < 8)) {
    web.send(400, "text/plain; charset=utf-8",
             WebMessages::text("Encryption requires a privacy password (8–64 bytes).", cfg.adminLanguage == "en"));
    return;
  }
  const Config previousConfig = cfg;
  const unsigned monitorIndex = activeMachine;
  const MachineProfile previousProfile = machines[edit];
  if (web.hasArg("language")) {
    String lang = web.arg("language"), color = web.arg("clockColor");
    bool valid = (lang == "zh" || lang == "en") && color.length() == 7 && color[0] == '#' &&
                 web.arg("alarmDays").toInt() > 0 && web.arg("alarmDays").toInt() <= 127 &&
                 web.arg("alarmTune").toInt() >= 0 && web.arg("alarmTune").toInt() <= 3;
    for (unsigned i = 1; i < color.length(); i++)
      if (!isxdigit(color[i]))
        valid = false;
    if (!valid) {
      web.send(400, "text/plain; charset=utf-8", WebMessages::text("Invalid display/alarm settings", cfg.adminLanguage == "en"));
      return;
    }
    String minute = web.hasArg("minuteColor") ? web.arg("minuteColor") : color;
    bool colorValid = minute.length() == 7 && minute[0] == '#';
    for (unsigned i = 1; i < minute.length(); i++)
      if (!isxdigit(minute[i]))
        colorValid = false;
    if (!colorValid || (web.hasArg("albumFixed") && web.arg("albumFixed") != "0" &&
                        web.arg("albumFixed") != "1")) {
      web.send(400, "text/plain; charset=utf-8", WebMessages::text("Invalid display settings", cfg.adminLanguage == "en"));
      return;
    }
    if (web.hasArg("showSeconds")) {
      if (web.arg("showSeconds") != "0" && web.arg("showSeconds") != "1") {
        web.send(400, "text/plain; charset=utf-8", WebMessages::text("Invalid seconds setting", cfg.adminLanguage == "en"));
        return;
      }
      cfg.clockSecondsEnabled = web.arg("showSeconds").toInt();
    }
    cfg.clockMinuteColor = minute;
    if (web.hasArg("albumFixed"))
      cfg.photoFixedMode = web.arg("albumFixed").toInt();
    cfg.displayLanguage = lang;
    cfg.clockHourColor = color;
    cfg.alarmWeekdayMask = web.arg("alarmDays").toInt();
    cfg.alarmMelody = web.arg("alarmTune").toInt();
  }
  if (web.hasArg("nasBackground"))
    cfg.monitorBackground = web.arg("nasBackground");
  if (web.hasArg("cardBackground")) cfg.monitorCardBackground = web.arg("cardBackground");
  cfg.snmpVersion = version;
  cfg.snmpUsername = user;
  cfg.snmpAuthPassword = authPassword;
  cfg.snmpAuthProtocol = authProtocol;
  cfg.snmpPrivacyProtocol = privacy;
  cfg.snmpPrivacyPassword = privatePassword;
  cfg.wifiSsid = ss;
  if (web.arg("pass").length())
    cfg.wifiPassword = web.arg("pass");
  cfg.snmpCommunity = web.arg("community").length() ? web.arg("community") : editing.snmpCommunity;
  if (web.arg("admin").length() && !Credentials::passwordValid(web.arg("admin"), cfg.adminPasswordVerifier)) {
    cfg.adminPasswordVerifier = web.arg("admin"); cfg.adminDefault = false;
  }
  cfg.touchSoundEnabled = web.arg("sound").toInt();
  cfg.alarmEnabled = web.arg("alarmEnabled").toInt();
  if (cfg.alarmTime != web.arg("alarmTime")) {
    LittleFS.remove("/alarm-day");
    alarmClock.lastDay = 0;
  }
  cfg.alarmTime = web.arg("alarmTime");
  cfg.alarmSnoozeMinutes = web.arg("snooze").toInt();
  cfg.photoIntervalSeconds = web.arg("albumInterval").toInt();
  cfg.weatherEnabled = web.arg("weatherEnabled").toInt();
  cfg.weatherCityCode = web.arg("cityCode");
  cfg.weatherRefreshMinutes = web.arg("weatherInterval").toInt();
  cfg.displayRotation = web.arg("rotation").toInt();
  cfg.timeServer = web.arg("ntpServer");
  cfg.startupPage = web.arg("home").toInt();
  cfg.snmpHost = host;
  cfg.storageVolumeIndex = st;
  cfg.networkInterfaceIndex = in;
  cfg.snmpPort = port;
  cfg.snmpPollIntervalSeconds = iv;
  cfg.displayBrightnessPercent = bl;
  if (web.hasArg("machineName")) {
    String name = web.arg("machineName"); name.trim();
    if (name.length() <= 48) machines[edit].name = name;
  }
  activeMachine = edit;
  captureMachine();
  applyMachine(monitorIndex);
  if (!save()) {
    cfg = previousConfig;
    machines[edit] = previousProfile;
    captureMachine();
    web.send(500, "text/plain; charset=utf-8", WebMessages::text("Configuration write failed", cfg.adminLanguage == "en"));
    return;
  }
  // Apply every saved setting from a clean boot. Device-list actions stay separate
  // so users can finish adding, removing and ordering profiles before restarting.
  const bool passwordChanged = cfg.adminPasswordVerifier != previousConfig.adminPasswordVerifier;
  web.sendHeader("X-MOS2-Save-Action", "restart");
  if (passwordChanged) {
    managementSessions.clear();
    web.sendHeader("Set-Cookie", "mos2_session=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
  }
  web.send(200, "text/plain; charset=utf-8",
           WebMessages::text("Saved. Restarting device.", cfg.adminLanguage == "en"));
  reboot = true;
  rebootAt = millis();
}
