#pragma once
#include <Updater.h>
#include "firmware_image.h"

// Referenced at boot so the linker retains the upload compatibility marker.
const uint8_t otaContract[] PROGMEM = {
  'M','O','S','2','O','T','A','1',0,0,0x20,0,
  uint8_t(Cjk::resourceCrc), uint8_t(Cjk::resourceCrc >> 8),
  uint8_t(Cjk::resourceCrc >> 16), uint8_t(Cjk::resourceCrc >> 24)
};
FirmwareImage::Validator firmwareValidator;
bool firmwareAccepted = false, firmwareKeepData = true, firmwareStarted = false;
uint8_t firmwareTail = 0;
String firmwareError;

void cancelFirmware(const char *reason) {
  firmwareError = reason;
  // One byte is always held back until validation succeeds; end(false) discards
  // incomplete staging without arming eboot or touching the running program.
  if (firmwareStarted) Update.end(false);
  firmwareStarted = false;
}
void receiveFirmware() {
  auto &u = web.upload();
  if (u.status == UPLOAD_FILE_START) {
    if (firmwareStarted) cancelFirmware("invalid");
    firmwareAccepted = false; firmwareError = "invalid";
    if (!requestOriginOK()) { firmwareError = "origin"; return; }
    if (!sessionValid()) { firmwareError = "login_required"; return; }
    if (cfg.adminDefault) { firmwareError = "password_change_required"; return; }
    if (web.header("X-CSRF-Token") != csrf) { firmwareError = "csrf"; return; }
    if (reboot) { firmwareError = "busy"; return; }
    String sizeText = web.arg("size"), crcText = web.arg("crc");
    if (!sizeText.length() || sizeText.length() > 7 || crcText.length() != 8) return;
    for (unsigned i=0;i<sizeText.length();++i) if (!isDigit(sizeText[i])) return;
    for (unsigned i=0;i<crcText.length();++i) if (!isxdigit(crcText[i])) return;
    size_t size = strtoul(sizeText.c_str(), nullptr, 10);
    firmwareValidator.begin(size, strtoul(crcText.c_str(), nullptr, 16), Cjk::resourceCrc);
    if (!firmwareValidator.valid || size > ESP.getFreeSketchSpace()) {
      firmwareError = "size"; return;
    }
    firmwareKeepData = web.arg("keep") != "0";
    if (!Update.begin(size, U_FLASH)) { firmwareError = "write"; return; }
    firmwareStarted = true; firmwareError = "";
  } else if (u.status == UPLOAD_FILE_WRITE && firmwareStarted) {
    size_t before = firmwareValidator.received;
    if (!firmwareValidator.add(u.buf, u.currentSize)) { cancelFirmware("invalid"); return; }
    size_t writable = u.currentSize;
    if (before + writable == firmwareValidator.expected && writable) {
      firmwareTail = u.buf[writable - 1]; --writable;
    }
    if (writable && Update.write(u.buf, writable) != writable) cancelFirmware("write");
    yield();
  } else if (u.status == UPLOAD_FILE_END && firmwareStarted) {
    if (!firmwareValidator.complete()) { cancelFirmware("invalid"); return; }
    if (Update.write(&firmwareTail, 1) != 1 || !Update.end()) {
      cancelFirmware("write"); return;
    }
    firmwareStarted = false;
    firmwareAccepted = true;
    if (!firmwareKeepData) eraseUserData();
    reboot = true; rebootAt = millis();
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    cancelFirmware("aborted");
  }
}
void finishFirmware() {
  if (!authorized()) return;
  if (web.header("X-CSRF-Token") != csrf) { web.send(403,"application/json","{\"error\":\"csrf\"}"); return; }
  web.sendHeader("Connection", "close");
  if (!firmwareAccepted) {
    if (!firmwareError.length()) firmwareError = "invalid";
    web.send(400,"application/json", "{\"error\":" + jsonString(firmwareError) + "}");
    return;
  }
  web.send(200,"application/json",String("{\"updated\":true,\"preserved\":") +
           (firmwareKeepData ? "true}" : "false}"));
}
