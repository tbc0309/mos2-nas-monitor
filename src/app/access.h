#pragma once
#include "access_policy.h"
// Password-only management sessions and the initial Wi-Fi setup flow.
// A per-boot session avoids browser Basic Authentication account prompts.
AccessPolicy::Sessions managementSessions;
String randomToken(unsigned words = 4) {
  String token;
  token.reserve(words * 8);
  char word[9];
  while (words--) { snprintf(word, sizeof word, "%08x", ESP.random()); token += word; }
  return token;
}
void secureHeaders() {
  if (web.uri() != "/weather-cities.js" && web.uri() != "/i18n.js" && web.uri() != "/management.js" && web.uri() != "/firmware-bundle.js")
    web.sendHeader("Cache-Control", "no-store");
  web.sendHeader("X-Content-Type-Options", "nosniff");
  web.sendHeader("X-Frame-Options", "DENY");
  web.sendHeader("Referrer-Policy", "same-origin");
}
bool requestOriginOK() {
  const String host = web.hostHeader();
  const String local = WiFi.localIP().toString(), portal = WiFi.softAPIP().toString();
  bool valid = host == local || host == local + ":80" ||
               (ap && (host == portal || host == portal + ":80"));
  const String origin = web.header("Origin");
  if (origin.length() && origin != "http://" + host) valid = false;
  if (!valid) web.send(403, "application/json", "{\"error\":\"origin\"}");
  return valid;
}
String sessionCookie() {
  String cookies = web.header("Cookie");
  int start = 0;
  while (start < int(cookies.length())) {
    int end = cookies.indexOf(';', start);
    if (end < 0) end = cookies.length();
    String part = cookies.substring(start, end); part.trim();
    if (part.startsWith("mos2_session=")) return part.substring(13);
    start = end + 1;
  }
  return "";
}
bool sessionValid() {
  return managementSessions.valid(sessionCookie().c_str(), millis());
}
bool authorized() {
  secureHeaders();
  if (!requestOriginOK()) return false;
  if (sessionValid()) {
    if (!cfg.adminDefault) { activity.demand(millis()); return true; }
    web.send(403, "application/json", "{\"error\":\"password_change_required\"}");
    return false;
  }
  web.send(401, "application/json", "{\"error\":\"login_required\"}");
  return false;
}
void entryPage(bool setup, const String &error = "") {
  // AP+STA consumes additional SDK RAM; avoid a second template transmit copy.
  web.client().setSync(true);
  web.client().setNoDelay(true);
  secureHeaders();
  // Stream the flash template: a complete page String needs >5KB contiguous heap.
  const String mode = setup ? "setup" : ((sessionValid() && cfg.adminDefault) ? "change" : "login");
  web.setContentLength(CONTENT_LENGTH_UNKNOWN);
  web.send(200, "text/html; charset=utf-8", "");
  unsigned start = 0, at = 0;
  while (pgm_read_byte(accessHTML + at)) {
    if (pgm_read_byte(accessHTML + at) != '{' || pgm_read_byte(accessHTML + at + 1) != '{') { ++at; continue; }
    if (at > start) sendFlashContent(accessHTML + start, at - start);
    String tag;
    unsigned end = at + 2;
    while (pgm_read_byte(accessHTML + end) && pgm_read_byte(accessHTML + end) != '}' && tag.length() < 24)
      tag += char(pgm_read_byte(accessHTML + end++));
    if (tag == "mode") web.sendContent(mode);
    else if (tag == "action") web.sendContent(setup ? "/wifi/save" : "/login");
    else if (tag == "token") web.sendContent(csrf);
    else if (tag == "wifiClass" && !setup) web.sendContent("hidden");
    else if (tag == "ssidDisabled" && !setup) web.sendContent("disabled");
    else if (tag == "entryTitle") web.sendContent(WebMessages::text(setup ? "Connect to Wi-Fi" : "Sign in", cfg.adminLanguage == "en"));
    else if (tag == "entrySubmit") web.sendContent(WebMessages::text(setup ? "Save & connect" : "Sign in", cfg.adminLanguage == "en"));
    else if (tag == "ssid" && cfg.wifiSsid.length()) web.sendContent(escapeHtml(cfg.wifiSsid));
    else if (tag == "error" && error.length()) web.sendContent(escapeHtml(error));
    at = end + 2; start = at;
    yield();
  }
  if (at > start) sendFlashContent(accessHTML + start, at - start);
  web.sendContent("");

}
void login() {
  secureHeaders();
  if (!requestOriginOK()) return;
  static AccessPolicy::LoginGuard guard;
  // An old form is not an incorrect password and must not consume the retry budget.
  if (web.arg("token") != csrf) { entryPage(false, "expired"); return; }
  if (guard.blocked(millis())) {
    web.sendHeader("Retry-After", String((guard.wait - uint32_t(millis() - guard.blockedAt) + 999) / 1000));
    entryPage(false, "wait");
    return;
  }
  if (!Credentials::passwordValid(web.arg("password"), cfg.adminPasswordVerifier)) {
    guard.failed(millis());
    entryPage(false, "wrong");
    return;
  }
  guard.clear();
  String managementSession = sessionCookie();
  if (!sessionValid()) {
    managementSession = randomToken(6);
    managementSessions.issue(managementSession.c_str(), millis());
  }
  web.sendHeader("Set-Cookie",
                 "mos2_session=" + managementSession + "; Path=/; HttpOnly; SameSite=Strict");
  web.sendHeader("Location", "/manage");
  web.send(303, "text/plain", "");
}
void logout() {
  if (!requestOriginOK()) return;
  if (!sessionValid()) {
    web.send(401, "application/json", "{\"error\":\"login_required\"}");
    return;
  }
  if (web.arg("token") != csrf) {
    web.send(403, "application/json", "{\"error\":\"csrf\"}");
    return;
  }
  managementSessions.revoke(sessionCookie().c_str());
  web.sendHeader("Set-Cookie", "mos2_session=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
  web.send(200, "application/json", "{\"signedOut\":true}");
}
// The default-password session can only replace that password, never access management APIs.
void changeDefaultPassword() {
  if (!requestOriginOK()) return;
  if (!sessionValid() || !cfg.adminDefault || web.arg("token") != csrf) {
    web.send(403, "application/json", "{\"error\":\"expired\"}");
    return;
  }
  String password = web.arg("newPassword");
  if (password.length() < 8 || password.length() > 64 || password == "12345678" ||
      password != web.arg("confirmPassword")) {
    web.send(400, "application/json", "{\"error\":\"passwordInvalid\"}");
    return;
  }
  String previous = cfg.adminPasswordVerifier;
  cfg.adminPasswordVerifier = password;
  cfg.adminDefault = false;
  if (!save()) {
    cfg.adminPasswordVerifier = previous;
    cfg.adminDefault = true;
    web.send(500, "application/json", "{\"error\":\"saveFailed\"}");
    return;
  }
  managementSessions.clear();
  web.sendHeader("Set-Cookie", "mos2_session=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
  web.send(200, "application/json", "{\"changed\":true}");
  lastDisplay = -1;
  requestDraw(); // Refresh the management QR/password notice immediately.
}
void wifiSave() {
  if (!requestOriginOK()) return;
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
  cfg.wifiSsid = ssid;
  cfg.wifiPassword = password;
  if (!save()) {
    cfg = previous;
    web.send(500, "application/json", "{\"error\":\"saveFailed\"}");
    return;
  }
  web.send(200, "application/json", "{\"saved\":true}");
  reboot = true;
  rebootAt = millis();
  rebootDelay = 6000; // Allow the captive browser to receive and paint the success instructions.
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
    // SDK scan allocation is asynchronous and does not report all OOM failures.
    // Defer discovery while HTTP/telemetry owns the necessary headroom.
    if (ESP.getFreeHeap() < 6500 || ESP.getMaxFreeBlockSize() < 3800) {
      web.send(503, "application/json", "{\"error\":\"scan_busy\"}");
      return;
    }
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
  // The browser owns the displayed list now. SDK scan records otherwise remain
  // allocated for the entire management session and starve configuration saves.
  WiFi.scanDelete();
  started = false;
}
