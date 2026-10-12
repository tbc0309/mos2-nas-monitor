#pragma once
#include "management/diagnostics_response.h"
// Register routes after the application handlers have been defined.
void registerWeb() {
  web.addHook([](const String &method, const String &url, WiFiClient* client, auto) {
    // Avoid retaining a second TCP transmit copy across UI requests.
    client->setSync(true);
    client->setNoDelay(true);
    RuntimeHealth::begin(method, url);
    // Parsing and upload callbacks complete in this handleClient call.
    web.releaseArgumentsWhenHandled();
    return ESP8266WebServer::CLIENT_REQUEST_CAN_CONTINUE;
  });
  web.collectHeaders("X-CSRF-Token", "Cookie", "Origin");
  web.on("/manage", HTTP_GET, home);
  web.on("/login", HTTP_POST, login);
  web.on("/password/change", HTTP_POST, changeDefaultPassword);
  web.on("/logout", HTTP_POST, logout);
  web.on("/wifi/save", HTTP_POST, wifiSave);
  web.on("/api/wifi/scan", HTTP_GET, wifiScan);
  web.on("/access-i18n.js", HTTP_GET,
         []() { sendFlash(200, "application/javascript; charset=utf-8", accessLocaleJS); });
  web.on("/access.js", HTTP_GET,
         []() { sendFlash(200, "application/javascript; charset=utf-8", accessJS); });
  web.on("/setup.js", HTTP_GET,
         []() { sendFlash(200, "application/javascript; charset=utf-8", setupJS); });
  web.on("/api/machine", HTTP_POST, []() {
    if (!authorized()) return;
    if (web.arg("token") != csrf) { web.send(403, "text/plain", "Reload page"); return; }
    String action = web.arg("action");
    if (action == "activate") {
      if (!switchMachine((activeMachine + 1) % machineCount, false)) { web.send(500, "text/plain", "Switch failed"); return; }
      web.send(200, "application/json", publicConfig()); return;
    } else if (action == "add") {
      if (machineCount >= MaxMachines) { web.send(400, "text/plain", "Maximum 6 devices"); return; }
      captureMachine();
      unsigned index = machineCount++;
      machines[index] = MachineProfile();
      auto &m = machines[index]; m.name = "";
      m.snmpHost = "192.168.1.100"; m.snmpCommunity = "public"; m.snmpVersion = "2c";
      m.snmpAuthProtocol = "MD5"; m.snmpPrivacyProtocol = "none";
      if (!save()) {
        machines[--machineCount] = MachineProfile();
        web.send(500, "text/plain", "Save failed"); return;
      }
      web.send(200, "application/json", publicConfig(index)); return;
    } else if (action == "move") {
      const String value = web.arg("index");
      const int separator = value.indexOf(':');
      unsigned from = 0, to = 0;
      if (separator < 1 || !AccessPolicy::decimal(value.substring(0, separator).c_str(), machineCount - 1, from) ||
          !AccessPolicy::decimal(value.substring(separator + 1).c_str(), machineCount - 1, to) || abs(int(from) - int(to)) != 1) {
        web.send(400, "text/plain", "Invalid order"); return;
      }
      captureMachine();
      const unsigned previous = activeMachine;
      MachineProfile temporary = machines[from]; machines[from] = machines[to]; machines[to] = temporary;
      if (activeMachine == unsigned(from)) activeMachine = to;
      else if (activeMachine == unsigned(to)) activeMachine = from;
      if (!save()) {
        temporary = machines[from]; machines[from] = machines[to]; machines[to] = temporary;
        activeMachine = previous;
        web.send(500, "text/plain", "Save failed"); return;
      }
    } else if (action == "delete") {
      if (machineCount <= 1) { web.send(400, "text/plain", "Keep one device"); return; }
      captureMachine();
      unsigned removedIndex = activeMachine;
      if (web.hasArg("index") && !AccessPolicy::decimal(web.arg("index").c_str(), machineCount - 1, removedIndex)) { web.send(400, "text/plain", "Invalid device"); return; }
      const unsigned previousActive = activeMachine;
      const MachineProfile removed = machines[removedIndex];
      for (unsigned i = removedIndex; i + 1 < machineCount; i++) machines[i] = machines[i + 1];
      machines[--machineCount] = MachineProfile();
      const unsigned next = previousActive == removedIndex ? 0 : previousActive > removedIndex ? previousActive - 1 : previousActive;
      applyMachine(next);
      if (!save()) {
        for (unsigned i = machineCount; i > removedIndex; i--) machines[i] = machines[i - 1];
        machines[removedIndex] = removed; machineCount++; applyMachine(previousActive);
        web.send(500, "text/plain", "Save failed"); return;
      }
      if (previousActive == removedIndex) { switchMachine(next, false); machineSelectionDirty = false; }
    } else if (action == "select") {
      String value = web.arg("index");
      unsigned index = 0;
      if (!AccessPolicy::decimal(value.c_str(), machineCount - 1, index)) {
        web.send(400, "text/plain", "Invalid device"); return;
      }
      web.send(200, "application/json", publicConfig(index)); return;
    } else { web.send(400, "text/plain", "Invalid action"); return; }
    web.send(200, "application/json", publicConfig());
  });
  registerExtras();
  web.on("/api/firmware", HTTP_POST, finishFirmware, receiveFirmware);
  web.on("/api/diagnostics", HTTP_GET, managementDiagnostics);
  web.on("/weather-cities.js", HTTP_GET, []() {
    if (!authorized()) return;
    web.sendHeader("Cache-Control", "private, max-age=86400");
    web.sendHeader("Content-Encoding", "gzip");
    sendFlash(200, "application/javascript; charset=utf-8", weatherCitiesJSGzip, sizeof(weatherCitiesJSGzip));
  });
  web.on("/i18n.js", HTTP_GET, []() {
    if (!authorized())
      return;
    web.sendHeader("Cache-Control", "private, max-age=31536000, immutable");
    web.sendHeader("Content-Encoding", "gzip");
    sendFlash(200, "application/javascript; charset=utf-8", localeJSGzip, sizeof(localeJSGzip));
  });
  web.on("/firmware-bundle.js", HTTP_GET, []() {
    if (!authorized()) return;
    web.sendHeader("Cache-Control", "private, max-age=31536000, immutable");
    web.sendHeader("Content-Encoding", "gzip");
    sendFlash(200, "application/javascript; charset=utf-8", firmwareBundleJSGzip, sizeof(firmwareBundleJSGzip));
  });
  web.on("/management.js", HTTP_GET, []() {
    if (!authorized())
      return;
    web.sendHeader("Cache-Control", "private, max-age=31536000, immutable");
    web.sendHeader("Content-Encoding", "gzip");
    sendFlash(200, "application/javascript; charset=utf-8", managementJSGzip, sizeof(managementJSGzip));
  });
  web.on("/", HTTP_GET, home);
  web.on("/save", HTTP_POST, settings);
  web.on("/api/status", HTTP_GET, status);
  web.on("/api/weather/check", HTTP_POST, WeatherValidation::check);
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
    paintAt = 0; // Reply before expensive screen painting; the main loop draws next.
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
