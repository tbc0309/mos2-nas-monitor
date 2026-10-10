#pragma once
// Password-only management sessions and the initial Wi-Fi setup flow.
// A per-boot session avoids browser Basic Authentication account prompts.
String managementSession;
uint32_t sessionSeen = 0;
bool sessionValid() {
  if (!managementSession.length() || uint32_t(millis() - sessionSeen) > 1800000)
    return false;
  String cookies = web.header("Cookie");
  int start = 0;
  while (start < int(cookies.length())) {
    int end = cookies.indexOf(';', start);
    if (end < 0)
      end = cookies.length();
    String part = cookies.substring(start, end);
    part.trim();
    if (part == "mos2_session=" + managementSession) {
      sessionSeen = millis();
      return true;
    }
    start = end + 1;
  }
  return false;
}
bool authorized() {
  if (sessionValid())
    return true;
  web.send(401, "application/json", "{\"error\":\"login_required\"}");
  return false;
}
void entryPage(bool setup, const String &error = "") {
  web.sendHeader("Cache-Control", "no-store");
  String h = FPSTR(accessHTML);
  h.replace("{{mode}}", setup ? "setup" : "login");
  h.replace("{{action}}", setup ? "/wifi/save" : "/login");
  h.replace("{{token}}", csrf);
  h.replace("{{ssid}}", escapeHtml(cfg.ssid));
  h.replace("{{error}}", escapeHtml(error));
  web.send(200, "text/html; charset=utf-8", h);
}
void login() {
  static unsigned failures = 0;
  static uint32_t blockedAt = 0;
  if (failures >= 5 && uint32_t(millis() - blockedAt) < 30000) {
    entryPage(false, "wait");
    return;
  }
  if (web.arg("token") != csrf || web.arg("password") != cfg.admin) {
    if (++failures >= 5)
      blockedAt = millis();
    entryPage(false, "wrong");
    return;
  }
  failures = 0;
  managementSession =
      String(ESP.random(), HEX) + String(ESP.random(), HEX) + String(ESP.random(), HEX);
  sessionSeen = millis();
  web.sendHeader("Set-Cookie",
                 "mos2_session=" + managementSession + "; Path=/; HttpOnly; SameSite=Strict");
  web.sendHeader("Location", "/manage");
  web.send(303, "text/plain", "");
}
void wifiSave() {
  if (!ap || web.arg("token") != csrf) {
    web.send(403, "application/json", "{\"error\":\"expired\"}");
    return;
  }
  String ssid = web.arg("ssid"), password = web.arg("password");
  if (!ssid.length() || ssid.length() > 32 || password.length() < 8 || password.length() > 63) {
    web.send(400, "application/json", "{\"error\":\"invalid\"}");
    return;
  }
  Config previous = cfg;
  cfg.ssid = ssid;
  cfg.pass = password;
  if (!save()) {
    cfg = previous;
    web.send(500, "application/json", "{\"error\":\"saveFailed\"}");
    return;
  }
  web.send(200, "application/json", "{\"saved\":true}");
  reboot = true;
  rebootAt = millis();
}

// Keep scans asynchronous so touch, display and captive DNS stay responsive.
void wifiScan() {
  if (!ap && !authorized())
    return;
  static bool started = false;
  static uint32_t scanAt = 0;
  int count = WiFi.scanComplete();
  if (!started || (web.arg("refresh") == "1" && count != WIFI_SCAN_RUNNING &&
                   uint32_t(millis() - scanAt) >= 5000)) {
    WiFi.scanDelete();
    WiFi.scanNetworks(true);
    scanAt = millis();
    started = true;
    web.send(200, "application/json", "{\"scanning\":true}");
    return;
  }
  if (count == WIFI_SCAN_RUNNING) {
    web.send(200, "application/json", "{\"scanning\":true}");
    return;
  }
  if (count < 0) {
    started = false;
    web.send(503, "application/json", "{\"error\":\"scan_failed\"}");
    return;
  }
  uint8_t order[128];
  count = min(count, 128);
  for (int i = 0; i < count; i++)
    order[i] = i;
  for (int i = 0; i < count; i++)
    for (int j = i + 1; j < count; j++)
      if (WiFi.RSSI(order[j]) > WiFi.RSSI(order[i])) {
        uint8_t tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
      }
  String result = "{\"scanning\":false,\"networks\":[";
  String seen[20];
  unsigned emitted = 0;
  for (int i = 0; i < count && emitted < 20; i++) {
    unsigned index = order[i];
    String ssid = WiFi.SSID(index);
    if (!ssid.length())
      continue;
    bool duplicate = false;
    for (unsigned j = 0; j < emitted; j++)
      if (seen[j] == ssid)
        duplicate = true;
    if (duplicate)
      continue;
    seen[emitted] = ssid;
    if (emitted++)
      result += ',';
    result +=
        "{\"ssid\":" + jsonString(ssid) + ",\"rssi\":" + String(WiFi.RSSI(index)) +
        ",\"open\":" + String(WiFi.encryptionType(index) == ENC_TYPE_NONE ? "true" : "false") + "}";
    yield();
  }
  result += "]}";
  web.send(200, "application/json", result);
}
