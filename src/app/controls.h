#pragma once
// Resolve each multi-click burst once; alarm controls and reset confirmation take priority.
void resetConfiguration() {
  eraseUserData();
  WiFi.disconnect(false);
  ESP.restart();
}
void stopAP() {
  if (ap) {
    dns.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    ap = false;
  }
}
void handleTouch(Touch::Action action) {
  if (action == Touch::None)
    return;
  unsigned count = unsigned(action);
  if (resetConfirm.pending) {
    if (resetConfirm.confirm(count, millis()))
      resetConfiguration();
    lastDisplay = -1;
    draw();
    return;
  }
  if (alarmClock.active() && (count == 1 || count == 2 || count == 3)) {
    if (count == 3) {
      alarmClock.taps = 0;
      for (int i = 0; i < 3; i++)
        alarmTap();
    } else if (count == 1)
      alarmTap();
    draw();
    return;
  }
  if (action == Touch::Reset) {
    touchButton.clearBurst();
    resetConfirm.begin(millis());
    lastDisplay = -1;
    draw();
    return;
  }
  if (cfg.sound)
    tone(12, 1760, 45);
  if (action == Touch::Setup)
    startAP();
  else if (action == Touch::SwitchMode) {
    stopAP();
    qrPage = false;
    page = lifestyle() ? 0 : 6;
    albumDrawn = -1;
  } else if (action == Touch::Management) {
    stopAP();
    qrPage = !qrPage;
  } else if (action == Touch::DoubleTap) {
    if (page == 8)
      screen.diskGroup = (screen.diskGroup + 1) % max(1u, (nasExtra.diskCount + 3) / 4);
    else if (page == 9)
      screen.volumeGroup = (screen.volumeGroup + 1) % max(1u, (nasExtra.volumeCount + 2) / 3);
    else if (page == 7 && albumCount) {
      albumPosition = (albumPosition + 1) % albumCount;
      albumAt = millis();
      albumDrawn = -1;
    }
  } else if (action == Touch::Tap) {
    if (ap) {
      stopAP();
    } else if (qrPage)
      qrPage = false;
    else
      nextPage();
  }
  lastDisplay = -1;
  paintAt = millis();
  draw();
}
