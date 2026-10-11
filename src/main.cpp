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
#include "nas_extra.h"
#include "network_interfaces.h"
#include "resource_metrics.h"
#include <TJpg_Decoder.h>
#include "animation_jpeg.h"

#include "app/state.h"
#include "flash_storage.h"
#include "app/config.h"
#include "app/display.h"
#include "app/access.h"
#include "app/web.h"
#include "app/monitor.h"
void resetConfiguration();
void handleTouch(Touch::Action action);
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
  Serial.println("\nMOS2 NAS 1.1.1 boot");
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
  Serial.printf("CONFIG loaded ssidLength=%u home=%u heap=%u\n", cfg.ssid.length(), cfg.home,
                ESP.getFreeHeap());
  brightness();
  tft.init();
  tft.setRotation(cfg.rotation);
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(jpegOutput);
  if (cfg.sound)
    tone(12, 1568, 80);
  apName = "MOS2-NAS-" + String(ESP.getChipId(), HEX);
  csrf = String(ESP.random(), HEX) + String(ESP.random(), HEX);
  apPass = "12345678";
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  if (cfg.ssid.length())
    WiFi.begin(cfg.ssid.c_str(), cfg.pass.c_str());
  else
    startAP();
  configTime(8 * 3600, 0, cfg.ntpServer.c_str(), "pool.ntp.org");
  storageIndex = cfg.storage;
  interfaceIndex = cfg.iface;
  discover = true;
  page = cfg.home;
  scanAlbum();
  {
    File f = LittleFS.open("/alarm-day", "r");
    if (f)
      alarmClock.lastDay = line(f).toInt();
  }
  registerWeb();
  Serial.printf("SNMP version=%s privacy=%s\n", cfg.snmpVersion.c_str(), cfg.snmpPrivacy.c_str());
  Serial.println("SETUP complete");
  draw();
}
#include "app/controls.h"

void loop() {
  web.handleClient();
  if (ap)
    dns.processNextRequest();
  if (reboot && millis() - rebootAt > rebootDelay)
    ESP.restart();
  static uint32_t disconnectedAt = 0;
  if (!ap && WiFi.status() != WL_CONNECTED) {
    if (!disconnectedAt)
      disconnectedAt = millis();
    if (millis() - disconnectedAt > 30000)
      startAP();
  } else
    disconnectedAt = 0;
  bool wasPending = resetConfirm.pending;
  resetConfirm.update(millis());
  if (wasPending && !resetConfirm.pending) {
    lastDisplay = -1;
    draw();
  }
  Touch::Action action = queuedTouch;
  queuedTouch = Touch::None;
  if (action == Touch::None)
    action = touchButton.update(!digitalRead(4), millis());
  handleTouch(action);
  monitor();
  weatherMonitor();
  if (!ap && WiFi.status() == WL_CONNECTED) {
    IPAddress ip;
    ip.fromString(cfg.nas);
    if (Telemetry::startup.ready(millis(), 8))
      networkInterfaces.update(ip, cfg.port, cfg.community);
    if (Telemetry::storageReady(millis()))
      nasExtra.update(ip, cfg.port, cfg.community, page == 8);
  }
  alarmService();
  if (millis() - paintAt > 1000) {
    paintAt = millis();
    draw();
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
  yield();
}
