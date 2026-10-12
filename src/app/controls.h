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
bool switchMachine(unsigned index, bool persist) {
  if (index >= machineCount) return false;
  const unsigned previous = activeMachine;
  captureMachine(); applyMachine(index);
  if (persist && !save()) { applyMachine(previous); return false; }
  machineSelectionDirty = !persist;
  machineSelectionAt = millis();
  metric = Metrics();
  // Reset confirmation and pending gestures belong to the previously selected target.
  resetNonce = "";
  touchButton.clearBurst();
  screen.historyCount = 0; screen.diskGroup = 0; screen.volumeGroup = 0;
  storageCollector.reset(); networkInterfaces.reset();
  storageIndex = cfg.storageVolumeIndex; interfaceIndex = cfg.networkInterfaceIndex;
  stopAP(); qrPage = false;
  // Desktop pages cannot render the monitor switch overlay. Enter overview first.
  if (page != 0 && page != 1 && page != 2 && page != 4 && page != 8 && page != 9) page = 0;
  // Device changes retain the current monitor page and reset only its data.
  Telemetry::reset();
  machineNotice = machines[index].name.length() ? machines[index].name : cfg.snmpHost;
  machineNoticeUntil = millis() + 1800;
  lastDisplay = -1;
  paintAt = 0;
  return true;
}
void handleTouch(Touch::Action action) {
  if (action == Touch::None)
    return;
  unsigned count = unsigned(action);
  if (action == Touch::DoubleTap && (resetConfirm.pending || alarmClock.active() || ap || qrPage || (page != 0 && page != 4))) ++touchBlocked;
  if (resetConfirm.pending) {
    if (resetConfirm.confirm(count, millis()))
      resetConfiguration();
    lastDisplay = -1;
    requestDraw();
    return;
  }
  if (alarmClock.active() && (count == 1 || count == 2 || count == 3)) {
    if (count == 3) {
      alarmClock.taps = 0;
      for (int i = 0; i < 3; i++)
        alarmTap();
    } else if (count == 1)
      alarmTap();
    requestDraw();
    return;
  }
  if (action == Touch::NextMachine) return; // Long holds have no navigation action.
  // Ignore gestures with no visible effect instead of forcing a complete repaint.
  if (action == Touch::DoubleTap &&
      (ap || qrPage || (page != 0 && page != 4 &&
       !(page == 8 && storageCollector.diskCount > 4) &&
       !(page == 9 && storageCollector.volumeCount > 3) &&
       !(page == 7 && albumCount > 1)))) return;
  if (action == Touch::DoubleTap && !ap && !qrPage && (page == 0 || page == 4)) {
    Serial.printf("TOUCH double: profiles=%u active=%u\n", machineCount, activeMachine);
    if (machineCount > 1) {
      if (switchMachine((activeMachine + 1) % machineCount, false)) {
        ++touchSwitches;
        if (cfg.touchSoundEnabled) {
          tone(12, 1760, 80);
          switchBeepAt = millis() + 160;
          switchBeepPending = true;
        }
        requestDraw();
      } else Serial.println("Device switch failed");
    }
    return;
  }
  if (action == Touch::Reset) {
    touchButton.clearBurst();
    resetConfirm.begin(millis());
    lastDisplay = -1;
    requestDraw();
    return;
  }
  if (cfg.touchSoundEnabled)
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
      screen.diskGroup = (screen.diskGroup + 1) % max(1u, (storageCollector.diskCount + 3) / 4);
    else if (page == 9)
      screen.volumeGroup = (screen.volumeGroup + 1) % max(1u, (storageCollector.volumeCount + 2) / 3);
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
  requestDraw();
}
