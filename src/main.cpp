#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <TFT_eSPI.h>
#include <qrcode.h>
#include "snmp.h"
#include "dashboard.h"
#include "webui.h"
#include "locales/web_messages.h"
#include "weather.h"
#include <time.h>
#include "touch.h"
#include "alarm.h"
#include "melodies.h"
#include "storage_collector.h"
#include "network_interfaces.h"
#include "resource_metrics.h"
#include <TJpg_Decoder.h>
#include "animation_jpeg.h"

#include "app/state.h"
#include "runtime_health.h"
#include "app/http_transfer.h"
#include "flash_storage.h"
#include "app/machines.h"
#include "credential_store.h"
#include "app/config.h"
#include "app/touch_input.h"
#include "app/display.h"
#include "app/access.h"
#include "app/weather_validation.h"
#include "app/web.h"
#include "app/monitor.h"
void resetConfiguration();
void handleTouch(Touch::Action action);
bool switchMachine(unsigned index, bool persist = true);
void eraseUserData() {
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    if (dir.isFile() && !Storage::isSystemResource(dir.fileName().c_str()))
      LittleFS.remove(dir.fileName());
  }
}
#include "app/extras_web.h"
#include "app/firmware_update.h"
#include "app/routes.h"
void setup() {
  Serial.begin(115200);
  Serial.printf("OTA layout=%02x\n", pgm_read_byte(otaContract + 10));
  Serial.println("\nMOS2 NAS 1.2.0 boot");
  Serial.printf("SDK %s\n", ESP.getSdkVersion());
  RuntimeHealth::previousCheckpoint();
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, OUTPUT);
  analogWriteRange(1023);
  load();
  Serial.printf("FONT filesystem resource %s bytes=%u\n", FontResource::begin() ? "OK" : "MISSING/INVALID", Cjk::resourceBytes);
  FSInfo fsInfo;
  if (LittleFS.info(fsInfo))
    Serial.printf("FILESYSTEM total=%u used=%u\n", fsInfo.totalBytes, fsInfo.usedBytes);
  Serial.printf("CONFIG loaded ssidLength=%u home=%u heap=%u\n", cfg.wifiSsid.length(), cfg.startupPage,
                ESP.getFreeHeap());
  brightness();
  tft.init();
  tft.setRotation(cfg.displayRotation);
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(jpegOutput);
  if (cfg.touchSoundEnabled)
    tone(12, 1568, 80);
  apName = "MOS2-NAS-" + String(ESP.getChipId(), HEX);
  csrf = randomToken();
  apPass = "12345678";
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  if (cfg.wifiSsid.length())
    WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPassword.c_str());
  else
    startAP();
  configTime(8 * 3600, 0, cfg.timeServer.c_str(), "pool.ntp.org");
  storageIndex = cfg.storageVolumeIndex;
  interfaceIndex = cfg.networkInterfaceIndex;
  discover = true;
  page = cfg.startupPage;
  scanAlbum();
  {
    File f = LittleFS.open("/alarm-day", "r");
    if (f)
      alarmClock.lastDay = line(f).toInt();
  }
  attachInterrupt(digitalPinToInterrupt(4), recordTouchEdge, CHANGE);
  registerWeb();
  Serial.printf("SNMP version=%s privacy=%s\n", cfg.snmpVersion.c_str(), cfg.snmpPrivacyProtocol.c_str());
  Serial.println("SETUP complete");
  draw();
}
#include "app/controls.h"

void loop() {
  RuntimeHealth::checkpoint(1);
  // Input is served before potentially slow HTTP and between background jobs.
  handleTouch(sampleTouch());
  RuntimeHealth::checkpoint(2);
  web.handleClient();
  web.releaseHandledArguments();
  RuntimeHealth::finish();
  // Finish delivering the save response, then reboot without painting a mixed
  // old/new palette or starting collectors during the restart grace period.
  if (reboot) {
    if (ap) dns.processNextRequest();
    if (millis() - rebootAt > rebootDelay) ESP.restart();
    yield();
    return;
  }
  handleTouch(sampleTouch());
  // Navigation renders after HTTP releases its request buffers, before any
  // background collector can postpone the user's requested page.
  if (displayDirty) {
    RuntimeHealth::checkpoint(7);
    displayDirty = false;
    paintAt = millis();
    draw();
    // An interrupted frame may have queued another gesture. Apply it before
    // starting more background work; the next loop paints its selected page.
    const Touch::Action nextGesture=sampleTouch();
    handleTouch(nextGesture);
    if (nextGesture!=Touch::None) { yield(); return; }
  }
  if (ap)
    dns.processNextRequest();
  if (reboot && millis() - rebootAt > rebootDelay)
    ESP.restart();
  // Configured Wi-Fi reconnects normally; provisioning is explicitly six-click only.
  bool wasPending = resetConfirm.pending;
  resetConfirm.update(millis());
  if (wasPending && !resetConfirm.pending) {
    lastDisplay = -1;
    draw();
  }
  activity.update(millis(), page);
  snmpCollectionEnabled = activity.snmp(millis());
  weatherCollectionEnabled = activity.weather(millis());
  RuntimeHealth::checkpoint(3);
  monitor();
  yield();
  handleTouch(sampleTouch());
  WeatherValidation::tick();
  RuntimeHealth::checkpoint(4);
  weatherMonitor();
  yield();
  handleTouch(sampleTouch());
  if (!ap && WiFi.status() == WL_CONNECTED && activity.snmp(millis())) {
    RuntimeHealth::checkpoint(5);
    IPAddress ip;
    ip.fromString(cfg.snmpHost);
    if (Telemetry::startup.ready(millis(), 8))
      networkInterfaces.update(ip, cfg.snmpPort, cfg.snmpCommunity);
    if (Telemetry::storageReady(millis()))
      // Synology's mount table can contain many container mounts. Discover its
      // disk health first so the first cache does not wait for that full walk.
      storageCollector.update(ip, cfg.snmpPort, cfg.snmpCommunity, page == 8, SnmpAdapter::policy(Telemetry::deviceProfile).includeRootVolume, SnmpAdapter::policy(Telemetry::deviceProfile).vendorDiskTable, page == 9);
  }
  yield();
  RuntimeHealth::checkpoint(6);
  handleTouch(sampleTouch());
  alarmService();
  if (machineSelectionDirty && millis() - machineSelectionAt >= 5000 && !weatherClient.busy()) {
    if (save()) machineSelectionDirty = false;
    else { ++selectionSaveFailures; machineSelectionAt = millis(); }
  }
  // Schedule the second confirmation beep without blocking input or SNMP polling.
  if (switchBeepPending && int32_t(millis() - switchBeepAt) >= 0) {
    switchBeepPending = false;
    if (cfg.touchSoundEnabled && !alarmClock.active()) tone(12, 1760, 80);
  }
  if (displayDirty || millis() - paintAt > 1000) {
    RuntimeHealth::checkpoint(7);
    displayDirty = false;
    paintAt = millis();
    draw();
  }
  static uint32_t cityPaintAt = 0;
  if (!ap && !qrPage && !resetConfirm.pending && page == 5 && millis()-cityPaintAt >= 100) {
    cityPaintAt = millis();
    paintWeatherCity();
  }
  static uint32_t animationAt = 0;
  static unsigned animationFrame = 0;
  if (!ap && !qrPage && !resetConfirm.pending && WiFi.status() == WL_CONNECTED && page == 5 &&
      millis() - animationAt >= 37) {
    animationAt = millis();
    animationJpeg = true;
    OriginalJpeg::draw(animationFrame);
    animationJpeg = false;
    animationDraws++;
    animationFrame = (animationFrame + 1) % 10;
  }
  RuntimeHealth::report(page, activeMachine);
  RuntimeHealth::checkpoint(8);
  yield();
}
