#pragma once
// Register routes after the application handlers have been defined.
void registerWeb() {
  web.collectHeaders("X-CSRF-Token", "Cookie");
  web.on("/manage", HTTP_GET, home);
  web.on("/login", HTTP_POST, login);
  web.on("/wifi/save", HTTP_POST, wifiSave);
  web.on("/api/wifi/scan", HTTP_GET, wifiScan);
  web.on("/access.js", HTTP_GET,
         []() { web.send_P(200, "application/javascript; charset=utf-8", accessJS); });
  web.on("/setup.js", HTTP_GET,
         []() { web.send_P(200, "application/javascript; charset=utf-8", setupJS); });
  registerExtras();
  web.on("/api/diagnostics", HTTP_GET, []() {
    if (!authorized())
      return;
    web.send(200, "application/json",
             String("{\"renderMicros\":") + renderMicros +
                 ",\"renderMaxMicros\":" + renderMaxMicros + ",\"bufferedDraws\":" + bufferedDraws +
                 ",\"displayBufferFailures\":" + displayBufferFailures + "," + snmp.diagnostics() +
                 ",\"animationFrames\":" + animationDraws + ",\"phase\":" + phase +
                 ",\"discover\":" + (discover ? "true" : "false") +
                 ",\"heap\":" + ESP.getFreeHeap() +
                 ",\"gateway\":" + jsonString(WiFi.gatewayIP().toString()) + "}");
  });
  web.on("/i18n.js", HTTP_GET, []() {
    if (!authorized())
      return;
    web.sendHeader("Cache-Control", "no-cache");
    web.send_P(200, "application/javascript; charset=utf-8", localeJS);
  });
  web.on("/management.js", HTTP_GET, []() {
    if (!authorized())
      return;
    web.sendHeader("Cache-Control", "no-cache");
    web.send_P(200, "application/javascript; charset=utf-8", managementJS);
  });
  web.on("/", HTTP_GET, home);
  web.on("/save", HTTP_POST, settings);
  web.on("/api/status", HTTP_GET, status);
  web.on("/api/page", HTTP_POST, []() {
    if (!authorized())
      return;
    if (web.arg("token") != csrf) {
      web.send(403, "text/plain", "Reload page");
      return;
    }
    int n = web.arg("page").toInt();
    if (n < 0 || n > 9 || n == 3 || (n == 5 && !cfg.weatherEnabled)) {
      web.send(400, "text/plain", "Invalid page");
      return;
    }
    page = n;
    qrPage = false;
    resetConfirm.cancel();
    lastDisplay = -1;
    albumDrawn = -1;
    draw();
    web.send(200, "application/json", "{}");
  });
  web.on("/api/beep", HTTP_POST, []() {
    if (!authorized())
      return;
    if (web.arg("token") != csrf) {
      web.send(403, "text/plain", "Reload page");
      return;
    }
    tone(12, 1760, 80);
    web.send(200, "application/json", "{}");
  });
  web.onNotFound([]() {
    if (ap) {
      web.sendHeader("Location", "http://192.168.4.1/", true);
      web.send(302, "text/plain", "");
    } else
      web.send(404, "text/plain", "Not found");
  });
  web.begin();
}
