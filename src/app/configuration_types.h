#pragma once
#include <Arduino.h>
constexpr uint8_t MaxMachines = 6;
// Values with Minutes/Seconds/Percent suffixes use those units throughout C++.
struct Config {
  bool adminDefault = true;
  String wifiSsid, wifiPassword, snmpHost = "192.168.1.100", snmpCommunity = "public", adminPasswordVerifier, storageVolumeIndex = "", networkInterfaceIndex = "";
  String snmpVersion = "2c", snmpUsername, snmpAuthPassword, snmpAuthProtocol = "MD5", snmpPrivacyProtocol = "none", snmpPrivacyPassword;
  String weatherCityCode = "101030100", timeServer = "ntp.aliyun.com";
  unsigned weatherEnabled = 1, weatherRefreshMinutes = 20, displayRotation = 0, touchSoundEnabled = 1;
  String alarmTime = "07:00";
  unsigned alarmEnabled = 0, alarmSnoozeMinutes = 5, photoIntervalSeconds = 10;
  String displayLanguage = "zh", clockHourColor = "#ffffff";
  String monitorBackground = "#21455c";
  String monitorCardBackground = "#103040";
  String clockMinuteColor = "#ffa500", adminLanguage = "zh";
  unsigned photoFixedMode = 0, clockSecondsEnabled = 1;
  unsigned alarmWeekdayMask = 31, alarmMelody = 0;
  unsigned startupPage = 6;
  unsigned snmpPort = 161, snmpPollIntervalSeconds = 5, displayBrightnessPercent = 70;
};
// Bounded profiles keep RAM use predictable on ESP8266.
struct MachineProfile {
  String monitorBackground = "#21455c";
  String monitorCardBackground = "#103040";
  String name, snmpHost, snmpCommunity, snmpVersion, snmpUsername, snmpAuthPassword, snmpAuthProtocol;
  String snmpPrivacyProtocol, snmpPrivacyPassword, networkInterfaceIndex, storageVolumeIndex;
  unsigned snmpPort = 161, snmpPollIntervalSeconds = 5;
};
