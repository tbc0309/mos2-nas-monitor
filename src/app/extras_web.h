#pragma once
String diskStatus() {
  String j = "[";
  for (unsigned i = 0; i < nasExtra.diskCount; i++) {
    const auto &d = nasExtra.disks[i];
    if (i)
      j += ',';
    j += "{\"name\":" + jsonString(d.name) + ",\"model\":" + jsonString(d.model) +
         ",\"temperature\":" + String(d.temp) + ",\"status\":" + String(d.status) +
         ",\"health\":" + String(d.health) + "}";
  }
  return j + "]";
}
String volumeStatus() {
  String j = "[";
  for (unsigned i = 0; i < nasExtra.volumeCount; i++) {
    const auto &d = nasExtra.volumes[i];
    if (i)
      j += ',';
    j += "{\"name\":" + jsonString(d.name) + ",\"total\":" + String((double)(d.size * d.units), 0) +
         ",\"used\":" + (d.valid ? String((double)(d.used * d.units), 0) : String("null")) +
         ",\"valid\":" + (d.valid ? "true" : "false") + "}";
  }
  return j + "]";
}
String alarmStatus() {
  time_t now = time(nullptr);
  struct tm local;
  localtime_r(&now, &local);
  return "{\"enabled\":" + String(cfg.alarmEnabled) + ",\"time\":" + jsonString(cfg.alarmTime) +
         ",\"state\":" + String(alarmClock.state) + ",\"days\":" + String(cfg.alarmDays) +
         ",\"weekday\":" + String((local.tm_wday + 6) % 7) +
         ",\"lastDay\":" + String(alarmClock.lastDay) + ",\"remaining\":" +
         String(alarmClock.state == Alarm::Snoozed && alarmClock.due > uint32_t(time(nullptr))
                    ? alarmClock.due - time(nullptr)
                    : 0) +
         "}";
}
bool tokenOK() {
  if (!authorized())
    return false;
  if (web.arg("token") == csrf || web.header("X-CSRF-Token") == csrf)
    return true;
  web.send(403, "text/plain; charset=utf-8", WebMessages::text("Reload page", cfg.adminLanguage == "en"));
  return false;
}
bool slotTextOK(const String &s) {
  return (s.length() == 1 && s[0] >= '0' && s[0] <= '6');
}
void albumList() {
  if (!authorized())
    return;
  String j = "[";
  for (int i = 0; i < albumCount; i++) {
    if (i)
      j += ',';
    j += "{\"slot\":" + String(albumSlots[i]) +
         ",\"url\":\"/api/album/image?slot=" + String(albumSlots[i]) + "\"}";
  }
  web.send(200, "application/json", j + "]");
}
void uploadPhoto() {
  auto &u = web.upload();
  if (u.status == UPLOAD_FILE_START) {
    uploadBad = true;
    uploadBytes = 0;
    uploadSlot = -1;
    if (!authorized())
      return;
    if (web.header("X-CSRF-Token") != csrf)
      return;
    String slot = web.arg("slot");
    if (web.arg("mode") == "add") {
      int next = 0;
      while (next < 7 && LittleFS.exists(photoPath(next)))
        next++;
      if (next == 7)
        return;
      uploadSlot = next;
    } else {
      if (!slotTextOK(slot))
        return;
      uploadSlot = slot.toInt();
    }
    FSInfo info;
    if (!LittleFS.info(info) || info.totalBytes - info.usedBytes < 49152 + 32768)
      return;
    albumUpload = LittleFS.open("/incoming.jpg", "w");
    uploadBad = !albumUpload;
  } else if (u.status == UPLOAD_FILE_WRITE) {
    uploadBytes += u.currentSize;
    if (uploadBytes > 49152)
      uploadBad = true;
    if (!uploadBad && albumUpload.write(u.buf, u.currentSize) != u.currentSize)
      uploadBad = true;
  } else if (u.status == UPLOAD_FILE_END) {
    if (albumUpload)
      albumUpload.close();
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    if (albumUpload)
      albumUpload.close();
    LittleFS.remove("/incoming.jpg");
    uploadBad = true;
  }
}
void commitPhoto() {
  if (!tokenOK())
    return;
  if (web.arg("mode") == "add" && albumCount >= 7) {
    LittleFS.remove("/incoming.jpg");
    web.send(409, "text/plain; charset=utf-8",
             WebMessages::text("Album is full (7 photos). Select a photo to replace.",
                               cfg.adminLanguage == "en"));
    return;
  }
  int slot = uploadSlot;
  if (web.arg("mode") != "add" && !slotTextOK(web.arg("slot")))
    uploadBad = true;
  uint16_t w = 0, h = 0;
  uint8_t result = 1;
  if (!uploadBad) {
    TJpgDec.getFsJpgSize(&w, &h, "/incoming.jpg", LittleFS);
    if (w == 240 && h == 240) {
      jpegValidate = true;
      result = TJpgDec.drawFsJpg(0, 0, "/incoming.jpg", LittleFS);
      jpegValidate = false;
    }
    if (w != 240 || h != 240 || result != 0)
      uploadBad = true;
  }
  if (uploadBad) {
    LittleFS.remove("/incoming.jpg");
    web.send(400, "text/plain; charset=utf-8",
             WebMessages::text("Use a 240x240 baseline JPEG, max 48KB. Original photo kept.",
                               cfg.adminLanguage == "en"));
    return;
  }
  String target = photoPath(slot);
  bool old = LittleFS.exists(target);
  LittleFS.remove("/photo-backup.jpg");
  if (old && !LittleFS.rename(target, "/photo-backup.jpg")) {
    LittleFS.remove("/incoming.jpg");
    web.send(500, "text/plain; charset=utf-8", WebMessages::text("Storage error", cfg.adminLanguage == "en"));
    return;
  }
  if (!LittleFS.rename("/incoming.jpg", target)) {
    if (old)
      LittleFS.rename("/photo-backup.jpg", target);
    web.send(500, "text/plain; charset=utf-8", WebMessages::text("Storage error", cfg.adminLanguage == "en"));
    return;
  }
  LittleFS.remove("/photo-backup.jpg");
  scanAlbum();
  web.send(200, "application/json", "{}");
}
void registerExtras() {
  web.on("/api/language", HTTP_POST, []() {
    if (!tokenOK())
      return;
    String language = web.arg("language");
    if (language != "zh" && language != "en") {
      web.send(400, "text/plain; charset=utf-8", WebMessages::text("Invalid language", cfg.adminLanguage == "en"));
      return;
    }
    String previousLanguage = cfg.adminLanguage;
    cfg.adminLanguage = language;
    if (!save()) {
      cfg.adminLanguage = previousLanguage;
      web.send(500, "text/plain; charset=utf-8", WebMessages::text("Configuration write failed", cfg.adminLanguage == "en"));
      return;
    }
    lastDisplay = -1;
    draw();
    web.send(200, "application/json", "{}");
  });
  web.on("/api/album", HTTP_GET, albumList);
  web.on("/api/album/image", HTTP_GET, []() {
    if (!authorized())
      return;
    String n = web.arg("slot");
    if (!slotTextOK(n)) {
      web.send(400, "text/plain; charset=utf-8", WebMessages::text("Invalid slot", cfg.adminLanguage == "en"));
      return;
    }
    File f = LittleFS.open(photoPath(n.toInt()), "r");
    if (!f) {
      web.send(404, "text/plain; charset=utf-8", WebMessages::text("No photo", cfg.adminLanguage == "en"));
      return;
    }
    web.sendHeader("Cache-Control", "no-store");
    web.streamFile(f, "image/jpeg");
    f.close();
  });
  web.on("/api/album/upload", HTTP_POST, commitPhoto, uploadPhoto);
  web.on("/api/album/delete", HTTP_POST, []() {
    if (!tokenOK())
      return;
    String n = web.arg("slot");
    if (!slotTextOK(n)) {
      web.send(400, "text/plain; charset=utf-8", WebMessages::text("Invalid slot", cfg.adminLanguage == "en"));
      return;
    }
    LittleFS.remove(photoPath(n.toInt()));
    scanAlbum();
    web.send(200, "application/json", "{}");
  });
  web.on("/api/alarm/test", HTTP_POST, []() {
    if (!tokenOK())
      return;
    testTune = web.hasArg("tune") ? constrain(web.arg("tune").toInt(), 0L, 3L) : cfg.alarmTune;
    alarmTest = true;
    alarmClock.state = Alarm::Ringing;
    alarmClock.ringStarted = time(nullptr);
    alarmClock.taps = 0;
    page = 6;
    qrPage = false;
    draw();
    web.send(200, "application/json", alarmStatus());
  });
  web.on("/api/alarm/tap", HTTP_POST, []() {
    if (!tokenOK())
      return;
    if (!alarmClock.active()) {
      web.send(400, "text/plain; charset=utf-8", WebMessages::text("No active alarm", cfg.adminLanguage == "en"));
      return;
    }
    alarmTap();
    draw();
    web.send(200, "application/json", alarmStatus());
  });
  web.on("/api/touch", HTTP_POST, []() {
    if (!tokenOK())
      return;
    String a = web.arg("action");
    int count = web.hasArg("count") ? web.arg("count").toInt()
                : a == "tap"        ? 1
                : a == "double"     ? 2
                : a == "triple"     ? 3
                : a == "four"       ? 4
                : a == "five"       ? 5
                : a == "six"        ? 6
                : a == "seven"      ? 7
                : a == "eight"      ? 8
                                    : 0;
    if (count < 1 || count > 8 || count == 4 || count == 7) {
      web.send(400, "text/plain; charset=utf-8", WebMessages::text("Invalid click count", cfg.adminLanguage == "en"));
      return;
    }
    handleTouch(Touch::Action(count));
    web.send(200, "application/json", "{}");
  });
  web.on("/api/reset/request", HTTP_POST, []() {
    if (!tokenOK())
      return;
    resetNonce = String(ESP.random(), HEX) + String(ESP.random(), HEX);
    resetNonceAt = millis();
    web.send(200, "application/json", "{\"nonce\":" + jsonString(resetNonce) + "}");
  });
  web.on("/api/reset/confirm", HTTP_POST, []() {
    if (!tokenOK())
      return;
    if (!resetNonce.length() || web.arg("nonce") != resetNonce || millis() - resetNonceAt > 15000) {
      web.send(403, "text/plain; charset=utf-8", WebMessages::text("Reset confirmation expired", cfg.adminLanguage == "en"));
      return;
    }
    resetNonce = "";
    web.send(200, "application/json", "{}");
    eraseUserData();
    reboot = true;
    rebootAt = millis();
  });
}
