#include "snmp/identity_label.h"
String automaticDeviceName();
#include "weather_locations.h"
#pragma once
#include "storage_order.h"
#include "render_policy.h"
#include "dirty_bands.h"
// Display refresh and alarm/photo presentation. Avoid full redraws between seconds.
void brightness() {
  analogWrite(5, 1023 - cfg.displayBrightnessPercent * 1023 / 100);
}
String address() {
  return "http://" + (ap ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "/";
}
void showQR(const String &text, int x, int y, int scale) {
  QRCode q;
  uint8_t data[512];
  qrcode_initText(&q, data, 5, ECC_LOW, text.c_str());
  int span = (q.size + 8) * scale;
  tft.fillRect(x, y, span, span, TFT_WHITE);
  for (int row = 0; row < q.size; row++)
    for (int col = 0; col < q.size; col++)
      if (qrcode_getModule(&q, col, row))
        tft.fillRect(x + (col + 4) * scale, y + (row + 4) * scale, scale, scale, TFT_BLACK);
}
bool animationJpeg = false;
bool jpegOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *pixels) {
  if (jpegValidate)
    return true;
  if (y >= 240)
    return false;
  if (animationJpeg) {
    for (unsigned i = 0; i < unsigned(w) * h; i++) {
      uint16_t v = (pixels[i] << 8) | (pixels[i] >> 8);
      if (((v >> 11) & 31) <= 3 && ((v >> 5) & 63) <= 6 && (v & 31) <= 3) {
        uint16_t bg = Dashboard::palette[Dashboard::BG];
        pixels[i] = (bg << 8) | (bg >> 8);
      }
    }
  }
  tft.pushImage(x, y, w, h, pixels);
  return true;
}
String photoPath(int slot) {
  return "/photo-" + String(slot) + ".jpg";
}
void scanAlbum() {
  albumCount = 0;
  for (int i = 0; i < 7; i++)
    if (LittleFS.exists(photoPath(i)))
      albumSlots[albumCount++] = i;
  albumPosition = 0;
  albumDrawn = -1;
  albumAt = millis();
}
void drawAlbum() {
  if (!albumCount)
    return;
  if (!cfg.photoFixedMode && albumCount > 1 && millis() - albumAt >= cfg.photoIntervalSeconds * 1000) {
    albumPosition = (albumPosition + 1) % albumCount;
    albumAt = millis();
  }
  int slot = albumSlots[albumPosition];
  if (albumDrawn != slot) {
    TJpgDec.drawFsJpg(0, 0, photoPath(slot).c_str(), LittleFS);
    albumDrawn = slot;
    albumDraws++;
  }
}
bool lifestyle() {
  return page == 5 || page == 6 || page == 7;
}
void nextPage() {
  if (lifestyle()) {
    page = page == 6 ? (cfg.weatherEnabled ? 5 : 7) : page == 5 ? 7 : 6;
  } else {
    const unsigned pages[] = {0, 1, 2, 8, 9, 4};
    unsigned i = 0;
    while (i < 6 && pages[i] != page)
      i++;
    page = pages[(i + 1) % 6];
    screen.diskGroup = screen.volumeGroup = 0;
  }
  albumDrawn = -1;
}
void alarmTap() {
  alarmClock.tap(millis(), time(nullptr), cfg.alarmSnoozeMinutes);
  touchButton.clearBurst();
  noTone(12);
  if (alarmClock.state == Alarm::Dismissed)
    alarmTest = false;
}
void alarmService() {
  if (reboot)
    return;
  time_t epoch = time(nullptr);
  struct tm info;
  localtime_r(&epoch, &info);
  uint32_t day = (info.tm_year + 1900) * 10000 + (info.tm_mon + 1) * 100 + info.tm_mday;
  unsigned target =
      cfg.alarmTime.substring(0, 2).toInt() * 60 + cfg.alarmTime.substring(3, 5).toInt();
  if (alarmClock.update(cfg.alarmEnabled || alarmTest,
                        alarmTest ? 999 : info.tm_hour * 60 + info.tm_min, target, day, epoch,
                        cfg.alarmSnoozeMinutes, (cfg.alarmWeekdayMask & (1u << ((info.tm_wday + 6) % 7))) != 0)) {
    File f = LittleFS.open("/alarm-day", "w");
    if (f) {
      f.println(day);
      f.close();
    }
    page = 6;
    qrPage = false;
    lastDisplay = -1;
  }
  if (alarmClock.state == Alarm::Dismissed) alarmTest = false;
  melody.update(alarmClock.state == Alarm::Ringing, alarmTest ? testTune : cfg.alarmMelody, millis());
}
uint32_t weatherCityStarted = 0;
template<class Paint> bool paintSmallRegion(int x,int y,int width,int height,Paint paint);
void paintWeatherCity() {
  if (page != 5 || Cjk::width(screen.weatherCity, 3) <= Dashboard::CityWidth) return;
  screen.cityScrollMs = millis() - weatherCityStarted;
  static int previousOffset = -1;
  static String previousCity;
  int offset = Dashboard::cityOffset(Cjk::width(screen.weatherCity, 3)-Dashboard::CityWidth,
                                    screen.cityScrollMs);
  if (previousCity == screen.weatherCity && previousOffset == offset) return;
  Dashboard::indexed = false;
  Cjk::background = Dashboard::palette[Dashboard::BG];
  if (!paintSmallRegion(Dashboard::CityLeft,17,Dashboard::CityWidth,28,
      [] { Dashboard::weatherCity(frame,screen); })) return;
  previousCity = screen.weatherCity;
  previousOffset = offset;
}
void requestDraw() {
  // HTTP/touch actions only mark state. Render after the request and its form
  // buffers have unwound, avoiding nested decoding/drawing on the small stack.
  displayDirty = true;
}
template<class Paint> bool paintSmallRegion(int x,int y,int width,int height,Paint paint) {
  const int rows=RenderPolicy::tileHeight(ESP.getFreeHeap(),ESP.getMaxFreeBlockSize(),width);
  if(!rows) { displayDirty=true; return false; }
  frame.setColorDepth(16);
  if(!frame.createSprite(width,rows)) { displayDirty=true; return false; }
  Cjk::clipLeft=x; Cjk::clipRight=x+width;
  bool complete=true;
  for(int top=y;top<y+height;top+=rows) {
    int count=min(rows,y+height-top);
    Cjk::clipTop=top; Cjk::clipBottom=top+count;
    frame.setOrigin(-x,-top);
    frame.fillSprite(Dashboard::palette[Dashboard::BG]);
    paint();
    frame.pushSprite(x,top,0,0,width,count);
    pollTouchInput(); yield();
    if(TouchInput::actions.pending()) { displayDirty=true; complete=false; break; }
  }
  Cjk::clipLeft=0; Cjk::clipRight=240; Cjk::clipTop=0; Cjk::clipBottom=240;
  frame.deleteSprite();
  return complete;
}
void draw() {
  screen.machineNotice = int32_t(machineNoticeUntil - millis()) > 0 ? machineNotice : "";
  screen.machineLabel = "SNMP " + String(activeMachine + 1) + "/" + String(machineCount);
  Cjk::background = 0x222b;
  Dashboard::palette[Dashboard::BG] = 0x222b;
  if (page != 5 && page != 6 && page != 7) {
    unsigned bg = strtoul(cfg.monitorBackground.c_str() + 1, nullptr, 16);
    Dashboard::palette[Dashboard::BG] =
        ((bg >> 19) & 31) << 11 | ((bg >> 10) & 63) << 5 | ((bg >> 3) & 31);
  }
  unsigned panelRgb = strtoul(cfg.monitorCardBackground.c_str() + 1, nullptr, 16);
  Dashboard::palette[Dashboard::PANEL] = ((panelRgb >> 19) & 31) << 11 | ((panelRgb >> 10) & 63) << 5 | ((panelRgb >> 3) & 31);
  Ui::chinese = cfg.displayLanguage == "zh";
  unsigned rgb = strtoul(cfg.clockHourColor.c_str() + 1, nullptr, 16);
  Dashboard::palette[Dashboard::SOFTRED] =
      ((rgb >> 19) & 31) << 11 | ((rgb >> 10) & 63) << 5 | ((rgb >> 3) & 31);
  rgb = strtoul(cfg.clockMinuteColor.c_str() + 1, nullptr, 16);
  Dashboard::palette[Dashboard::SOFTGREEN] =
      ((rgb >> 19) & 31) << 11 | ((rgb >> 10) & 63) << 5 | ((rgb >> 3) & 31);
  screen.resetPending = resetConfirm.pending;

  int display = resetConfirm.pending            ? 4
                : ap                            ? 2
                : qrPage                        ? 1
                : WiFi.status() != WL_CONNECTED ? 3
                                                : 0;
  if (display && display == lastDisplay)
    return;
  if (display != lastDisplay) {
    desktopPaintKey = "";
    desktopPaintPage = -1;
  }
  bool displayChanged = display != lastDisplay;
  lastDisplay = display;
  if (resetConfirm.pending) {
    Dashboard::indexed = false;
    Dashboard::render(tft, screen, page);
    return;
  }
  if (ap || qrPage) {
    tft.fillScreen(Dashboard::palette[Dashboard::BG]);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(TL_DATUM);
    Ui::draw(tft, ap ? "WI-FI SETUP" : "MANAGEMENT", 120, 21, 3, Dashboard::c(Dashboard::CYAN),
             MC_DATUM);
    tft.setTextDatum(TL_DATUM);
    String payload = ap ? "WIFI:T:WPA;S:" + apName + ";P:" + apPass + ";;" : address();
    showQR(payload, 52, 42, 3);
    Ui::draw(tft, ap ? apName : WiFi.localIP().toString(), 120, 187, 2, TFT_WHITE, MC_DATUM);
    Ui::draw(tft, ap ? "Key: " + apPass : (cfg.adminDefault ? "Key: 12345678" : "Default password changed"), 120, 208, 2,
             Dashboard::c(ap ? Dashboard::ORANGE : Dashboard::CYAN), MC_DATUM);
    Ui::draw(tft, ap ? "Open 192.168.4.1" : "Touch to return", 120, 229, 2,
             Dashboard::c(ap ? Dashboard::CYAN : Dashboard::MUTED), MC_DATUM);
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    tft.fillScreen(Dashboard::palette[Dashboard::BG]);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(TL_DATUM);
    Ui::draw(tft, "CONNECTING", 120, 57, 3, Dashboard::c(Dashboard::CYAN), MC_DATUM);
    Ui::draw(tft, "Joining Wi-Fi...", 120, 102, 2, TFT_WHITE, MC_DATUM);
    Ui::draw(tft, "6 taps: Wi-Fi setup", 120, 148, 2, Dashboard::c(Dashboard::ORANGE), MC_DATUM);
    return;
  }
  screen.cpu = metric.cpu;
  screen.memory = metric.memory;
  screen.storage = metric.storage;
  screen.temp = metric.temp;
  screen.rx = metric.rx;
  screen.tx = metric.tx;
  screen.memoryTotal = metric.total * 1024;
  screen.memoryAvail = metric.avail * 1024;
  screen.volumeTotal = metric.size * metric.units;
  screen.volumeUsed = metric.used * metric.units;
  screen.source = cfg.snmpVersion == "3" ? "SNMPv3" : "SNMPv2c";
  screen.security = cfg.snmpVersion == "3" ? cfg.snmpAuthProtocol +
      (cfg.snmpPrivacyProtocol != "none" ? "+" + cfg.snmpPrivacyProtocol : String("")) : String("");
  screen.privacy = cfg.snmpVersion == "3" && cfg.snmpPrivacyProtocol != "none";
  screen.online = metric.online;
  screen.nas = cfg.snmpHost;
  screen.device = WiFi.localIP().toString();
  screen.model = machines[activeMachine].name.length() ? machines[activeMachine].name : automaticDeviceName();
  screen.volume = metric.volume;
  screen.iface = metric.iface;
  screen.rssi = WiFi.RSSI();
  screen.uptime = metric.uptime / 100;
  screen.age = metric.last ? (millis() - metric.last) / 1000 : 0;
  screen.interval = cfg.snmpPollIntervalSeconds;
  screen.weatherEnabled = cfg.weatherEnabled;
  screen.weatherOnline = weather.valid &&
                         millis() - weather.fetched <= max(1800000u, cfg.weatherRefreshMinutes * 180000u);
  screen.weatherTemp = weather.temperature;
  screen.humidity = weather.humidity;
  screen.aqi = weather.aqi;
  screen.weatherCode = weather.code;
  screen.weatherCity = WeatherLocations::listed(cfg.weatherCityCode)
      ? (Ui::chinese ? weather.city : weather.cityEnglish)
      : (weather.cityEnglish.length() ? weather.cityEnglish : String("Weather"));
  static String scrollCity;

  if (scrollCity != screen.weatherCity || desktopPaintPage != 5) {
    scrollCity = screen.weatherCity;
    weatherCityStarted = millis();
  }
  screen.cityScrollMs = millis() - weatherCityStarted;
  screen.weatherCondition = Ui::chinese ? weather.condition : weather.conditionEnglish;
  screen.weatherWind = Ui::chinese ? weather.wind : weather.windEnglish;
  screen.weatherObserved = weather.observed;
  time_t now = time(nullptr);
  if (now > 1700000000) {
    struct tm info;
    localtime_r(&now, &info);
    char clock[12], date[20];
    strftime(clock, sizeof(clock), "%H:%M:%S", &info);
    strftime(date, sizeof(date), "%Y-%m-%d", &info);
    screen.clock = clock;
    screen.date = date;
    screen.weekday = info.tm_wday;
  }
  screen.diskCount = storageCollector.diskCount;
  screen.volumeCount = storageCollector.volumeCount;
  // Read cached rows through a sorted index; do not duplicate their Strings or
  // reorder collector rows while requests and timestamp arrays refer to them.
  screen.disks.bind(storageCollector.disks, screen.diskCount, [](const Dashboard::Disk &d) { return d.index.c_str(); });
  screen.volumes.bind(storageCollector.volumes, screen.volumeCount, [](const Dashboard::Volume &v) { return v.name.c_str(); });
  screen.showSeconds = cfg.clockSecondsEnabled;
  screen.alarmEnabled = cfg.alarmEnabled;
  screen.alarmTime = cfg.alarmTime;
  screen.alarmState = alarmClock.state;
  screen.alarmRemaining = alarmClock.state == Alarm::Snoozed && alarmClock.due > uint32_t(now)
                              ? alarmClock.due - now
                              : 0;
  if (page != 5 && page != 6) {
    desktopPaintKey = "";
    desktopPaintPage = -1;
  }
  if (page == 7 && albumCount) {
    drawAlbum();
    return;
  }
  albumDrawn = -1;
  if (page == 5 || page == 6) {
    String key = screen.clock.substring(0, 5) + screen.date + String(screen.weekday) +
                 String(screen.alarmState) + String(screen.alarmEnabled) +
                 String(screen.alarmRemaining / 60);
    if (page == 5)
      key += String(weather.fetched);
    bool same = desktopPaintPage == int(page) && desktopPaintKey == key;
    if (page == 5 && desktopPaintPage != 5)
      tft.fillRect(160, 160, 80, 80, Dashboard::palette[Dashboard::BG]);
    desktopPaintPage = page;
    desktopPaintKey = key;
    if (same) {
      Dashboard::indexed = false;
      Cjk::background = Dashboard::palette[Dashboard::BG];
      static String desktopSecond;
      bool secondChanged = desktopSecond != screen.clock;
      if (secondChanged && screen.showSeconds && screen.clock.length() == 8) {
        if(!paintSmallRegion(180,110,44,34,[&]() {
          OriginalAssets::second(frame,182,112,screen.clock[6],Dashboard::WHITE);
          OriginalAssets::second(frame,202,112,screen.clock[7],Dashboard::WHITE);
        })) return;
      }
      desktopSecond=screen.clock;
      if (page == 5) {
        static String previousWeatherLine;
        String line=(screen.clock.length()==8 && (screen.clock[7]-'0')%6<3)?screen.weatherCondition:screen.weatherWind;
        if(previousWeatherLine!=line) {
          // Render the whole intersecting band to preserve any weather artwork.
          if(!paintSmallRegion(0,48,240,26,[&]() { Dashboard::render(frame,screen,page); })) return;
          previousWeatherLine=line;
        }
      }
      return;
    }
  }

  static DirtyBands::Plan previousBands;
  static uint32_t previousContext=0;
  static int nasPaintPage=-1;
  const bool nasPage=page==0 || page==1 || page==2 || page==8 || page==9 || page==4;
  DirtyBands::Plan currentBands;
  uint32_t context=0,dirtyMask=DirtyBands::All;
  if(nasPage) {
    context=DirtyBands::Plan::fingerprint(cfg.monitorCardBackground+cfg.monitorBackground+
      cfg.displayLanguage+cfg.snmpHost+screen.machineNotice+screen.machineLabel);
    currentBands=DirtyBands::monitor(screen,page);
    if(!displayChanged && nasPaintPage==int(page) && previousContext==context)
      dirtyMask=DirtyBands::difference(previousBands,currentBands);
    if(!dirtyMask) return;
  } else nasPaintPage=-1;
  // Small RGB565 tiles stay usable when SNMP and HTTP fragment the heap.
  // Falling back to direct full-screen clears caused visible overview flashes.
  uint32_t renderStarted = micros();
  frame.setColorDepth(16);
  // Leave headroom for TCP and credential operations; avoid failed large allocations.
  const uint32_t availableBlock = ESP.getMaxFreeBlockSize();
  const uint32_t availableHeap = ESP.getFreeHeap();
  int tileHeight = RenderPolicy::tileHeight(availableHeap,availableBlock);
  // Retain the previous complete screen while HTTP owns nearly all RAM.
  // Wi-Fi SDK beacon handling allocates outside our main loop and does not
  // handle allocation failure safely. Leave its reserve intact while painting.
  if (!tileHeight) {
    displayDirty = true;
    return;
  }
  Serial.printf("DRAW begin page=%u heap=%u block=%u tile=%d\n", page, availableHeap, availableBlock, tileHeight);
  bool buffered = frame.createSprite(240, tileHeight);
  if (!buffered) {
    tileHeight = 1;
    buffered = frame.createSprite(240, tileHeight);
  }
  if (buffered) {
    static bool logged = false;
    if (!logged) {
      Serial.printf("DISPLAY tile buffer OK freeHeap=%u\n", ESP.getFreeHeap());
      logged = true;
    }
    Dashboard::indexed = false;
    renderRows=0;
    for (int top = 0; top < 240; top += tileHeight) {
      const int rows=top+tileHeight>240 ? 240-top : tileHeight;
      if(nasPage && !DirtyBands::intersects(dirtyMask,top,rows)) continue;
      renderRows+=rows;
      Cjk::clipTop = top;
      Cjk::clipBottom = top + rows;
      frame.setOrigin(0, -top);
      frame.fillSprite(Dashboard::palette[Dashboard::BG]);
      Dashboard::render(frame, screen, page);
      if (page == 5 && top >= 160)
        frame.pushSprite(0, top, 0, 0, 160, rows);
      else if (page == 5 && top < 160 && top + rows > 160) {
        int upper = 160 - top;
        frame.pushSprite(0, top, 0, 0, 240, upper);
        frame.pushSprite(0, 160, 0, upper, 160, rows - upper);
      } else
        frame.pushSprite(0, top,0,0,240,rows);
      pollTouchInput();
      yield();
      if (TouchInput::actions.pending()) {
        Cjk::clipTop=0; Cjk::clipBottom=240; frame.deleteSprite();
        displayDirty=true; nasPaintPage=-1; desktopPaintPage=-1;
        return; // Let the main loop apply the queued gesture before painting more rows.
      }
    }
    Cjk::clipTop = 0;
    Cjk::clipBottom = 240;
    frame.deleteSprite();
    renderMicros = micros() - renderStarted;
    if (renderMicros > renderMaxMicros)
      renderMaxMicros = renderMicros;
    bufferedDraws++;
    if(nasPage && dirtyMask!=DirtyBands::All) ++partialDraws;
    Serial.printf("DRAW complete page=%u us=%u heap=%u\n", page, renderMicros, ESP.getFreeHeap());
    if (nasPage) {
      nasPaintPage = page;
      previousBands=currentBands;
      previousContext=context;
    }
  } else {
    // Keep the last complete image and retry; never clear the physical LCD mid-frame.
    displayBufferFailures++;
    desktopPaintPage = -1;
    nasPaintPage = -1;
  }
}
void sampleHistory() {
  Serial.printf("METRICS mode=%s cpu=%.1f mem=%.1f disk=%.1f temp=%.1f rx=%.1f tx=%.1f heap=%u\n",
                "snmp", metric.cpu, metric.memory, metric.storage, metric.temp, metric.rx,
                metric.tx, ESP.getFreeHeap());
  if (screen.historyCount == 30) {
    for (unsigned i = 1; i < 30; i++) {
      screen.rxHistory[i - 1] = screen.rxHistory[i];
      screen.txHistory[i - 1] = screen.txHistory[i];
    }
  } else
    screen.historyCount++;
  screen.rxHistory[screen.historyCount - 1] = metric.rx;
  screen.txHistory[screen.historyCount - 1] = metric.tx;
}
