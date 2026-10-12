#pragma once
#include "activity_policy.h"
Activity::Policy activity;
bool snmpCollectionEnabled = true, weatherCollectionEnabled = true;
// Single-owner application state; only main.cpp includes this header.
Resources::Cpu cpuTicks;
TFT_eSPI tft;
#include "management_server.h"
ManagementServer web(80);
DNSServer dns;
Weather::Client weatherClient;
Weather::Reading weather;
uint32_t weatherAttempt = 0;
bool weatherStarted = false;
unsigned weatherFailures = 0;
#include "configuration_types.h"
Config cfg;
MachineProfile machines[MaxMachines];
unsigned machineCount = 1, activeMachine = 0;
String machineNotice;
uint32_t machineNoticeUntil = 0;
bool switchBeepPending = false;
uint32_t switchBeepAt = 0;
struct Metrics {
  String model, volume, iface;
  float cpu = -1, memory = -1, storage = -1, temp = -1, rx = -1, tx = -1;
  uint32_t uptime = 0, last = 0;
  uint64_t total = 0, avail = 0, size = 0, units = 0, used = 0, oldRx = 0, oldTx = 0;
  uint32_t counterTime = 0;
  uint32_t cpuAt = 0, memoryAt = 0, tempAt = 0, storageAt = 0, networkAt = 0;
  bool counters = false, online = false;
} metric;
String apName, apPass, storageIndex, interfaceIndex, csrf;
Dashboard::State screen;
TFT_eSprite frame(&tft);
bool ap = false, qrPage = false, reboot = false, discover = false;
uint32_t paintAt = 0, rebootAt = 0, rebootDelay = 1200;
bool displayDirty = false;
int lastDisplay = -1;
Touch::ResetConfirmation resetConfirm;
Melodies::Player melody;
String resetNonce;
uint32_t resetNonceAt = 0;
bool machineSelectionDirty = false;
uint32_t machineSelectionAt = 0;
uint32_t touchSwitches = 0, touchBlocked = 0, selectionSaveFailures = 0;
Touch::Button touchButton;
Alarm::Clock alarmClock;
StorageCollector storageCollector;
NetworkInterfaces networkInterfaces;
bool alarmTest = false, jpegValidate = false;
unsigned testTune = 0;
uint32_t albumDraws = 0, animationDraws = 0;
uint32_t bufferedDraws = 0, displayBufferFailures = 0;
uint32_t renderMicros = 0, renderMaxMicros = 0;
uint32_t renderRows = 0, partialDraws = 0;
String desktopPaintKey;
int desktopPaintPage = -1;
int albumSlots[7], albumCount = 0, albumPosition = 0, albumDrawn = -1;
uint32_t albumAt = 0;
File albumUpload;
bool uploadBad = false;
size_t uploadBytes = 0;
int uploadSlot = -1;
unsigned phase = 0, page = 0;
