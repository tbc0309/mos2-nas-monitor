#pragma once
#include "access_policy.h"
// Only the selected profile is polled. Secrets never enter the public profile list.
void captureMachine() {
  auto &m = machines[activeMachine];
  m.monitorBackground = cfg.monitorBackground; m.monitorCardBackground = cfg.monitorCardBackground;
  m.snmpHost = cfg.snmpHost; m.snmpCommunity = cfg.snmpCommunity; m.snmpVersion = cfg.snmpVersion;
  m.snmpUsername = cfg.snmpUsername; m.snmpAuthPassword = cfg.snmpAuthPassword; m.snmpAuthProtocol = cfg.snmpAuthProtocol;
  m.snmpPrivacyProtocol = cfg.snmpPrivacyProtocol; m.snmpPrivacyPassword = cfg.snmpPrivacyPassword;
  m.networkInterfaceIndex = cfg.networkInterfaceIndex; m.storageVolumeIndex = cfg.storageVolumeIndex; m.snmpPort = cfg.snmpPort; m.snmpPollIntervalSeconds = cfg.snmpPollIntervalSeconds;
  // Empty name means automatic identity; never persist a display fallback.
}
void applyMachine(unsigned index) {
  activeMachine = index;
  const auto &m = machines[index];
  cfg.monitorBackground = m.monitorBackground; cfg.monitorCardBackground = m.monitorCardBackground;
  cfg.snmpHost = m.snmpHost; cfg.snmpCommunity = m.snmpCommunity; cfg.snmpVersion = m.snmpVersion;
  cfg.snmpUsername = m.snmpUsername; cfg.snmpAuthPassword = m.snmpAuthPassword; cfg.snmpAuthProtocol = m.snmpAuthProtocol;
  cfg.snmpPrivacyProtocol = m.snmpPrivacyProtocol; cfg.snmpPrivacyPassword = m.snmpPrivacyPassword;
  cfg.networkInterfaceIndex = m.networkInterfaceIndex; cfg.storageVolumeIndex = m.storageVolumeIndex; cfg.snmpPort = m.snmpPort; cfg.snmpPollIntervalSeconds = m.snmpPollIntervalSeconds;
}
void readMachines(JsonDocument &doc) {
  captureMachine();
  JsonArray list = doc["machines"].as<JsonArray>();
  if (!list.size() || list.size() > MaxMachines) return;
  machineCount = list.size();
  for (unsigned i = 0; i < machineCount; i++) {
    auto o = list[i]; auto &m = machines[i];
    m.monitorBackground = o["nasBackground"] | "#21455c"; m.monitorCardBackground = o["cardBackground"] | "#103040";
    m.name = o["name"].as<String>(); m.snmpHost = o["nas"].as<String>();
    m.snmpCommunity = o["community"].as<String>(); m.snmpVersion = o["snmpVersion"].as<String>();
    m.snmpUsername = o["snmpUser"].as<String>(); m.snmpAuthPassword = o["snmpPassword"].as<String>();
    m.snmpAuthProtocol = o["snmpAuth"].as<String>(); m.snmpPrivacyProtocol = o["snmpPrivacy"].as<String>();
    m.snmpPrivacyPassword = o["snmpPrivacyPassword"].as<String>();
    m.networkInterfaceIndex = o["iface"].as<String>(); m.storageVolumeIndex = o["storage"].as<String>();
    m.snmpPort = constrain(o["port"].as<unsigned>(), 1u, 65535u);
    m.snmpPollIntervalSeconds = constrain(o["interval"].as<unsigned>(), 3u, 60u);
    // Validate every profile, including inactive ones, before a future switch.
    IPAddress address;
    if (!address.fromString(m.snmpHost)) m.snmpHost = "192.168.1.100";
    m.name = m.name.substring(0, 48);

    m.snmpCommunity = m.snmpCommunity.substring(0, 64);
    m.snmpUsername = m.snmpUsername.substring(0, 32);
    m.snmpAuthPassword = m.snmpAuthPassword.substring(0, 64);
    m.snmpPrivacyPassword = m.snmpPrivacyPassword.substring(0, 64);
    if (m.snmpVersion != "3") m.snmpVersion = "2c";
    if (m.snmpAuthProtocol != "SHA") m.snmpAuthProtocol = "MD5";
    if (m.snmpPrivacyProtocol != "DES" && m.snmpPrivacyProtocol != "AES") m.snmpPrivacyProtocol = "none";
    unsigned index;
    if (m.networkInterfaceIndex.length() && !AccessPolicy::decimal(m.networkInterfaceIndex.c_str(), 2147483647u, index)) m.networkInterfaceIndex = "";
    if (m.storageVolumeIndex.length() && !AccessPolicy::decimal(m.storageVolumeIndex.c_str(), 2147483647u, index)) m.storageVolumeIndex = "";
  }
  applyMachine(min(doc["activeMachine"].as<unsigned>(), machineCount - 1));
}
