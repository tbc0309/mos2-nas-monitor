#pragma once
#include "snmp.h"
// Independent discovery, requested at boot/periodically and by management.
class NetworkInterfaces {
  Snmp::Client client;
  String walk, candidate;
  unsigned stage = 0, steps = 0, workingCount = 0;
  bool requested = false, running = false;
  uint32_t completed = 0, retryWait = Sampling::InterfacePeriod;
  bool failed = false, paused = false, bootstrap = true;
  const String root = "1.3.6.1.2.1.2.2.1.2";

public:
  struct Entry {
    String index, name;
    bool up = false;
  } entries[16], working[16];
  unsigned count = 0;
  void reset() {
    client.begin();
    requested = running = paused = false;
    completed = 0;
    count = 0; bootstrap = true;
  }
  void request() {
    requested = true;
  }
  void update(IPAddress ip, unsigned port, const String &community) {
    if (!running) {
      if (!requested || (completed && millis() - completed < retryWait))
        return;
      requested = false;
      failed = false;
      running = true;
      if (!paused) {
        walk = root;
        workingCount = steps = stage = 0;
      }
      paused = false;
    }
    Snmp::Value value;
    int result = client.poll(value);
    if (result) {
      if (result != 1 && stage == 0) {
        paused = requested = true; running = false;
        completed = millis(); retryWait = count ? Sampling::RetryCeiling : Sampling::DiscoveryRetry;
        return;
      }
      if (result != 1) failed = true;
      if (stage == 0) {
        if (result != 1 || !value.oid.startsWith(root + ".") || ++steps > 64 ||
            workingCount == 16) {
          if (workingCount && !failed) {
            count = workingCount;
            for (unsigned i = 0; i < count; i++)
              entries[i] = working[i];
          }
          running = false;
          completed = millis();
          retryWait = failed ? (count ? Sampling::RetryCeiling : Sampling::DiscoveryRetry) : Sampling::InterfacePeriod;
          if (failed) requested = true;
          return;
        }
        walk = value.oid;
        candidate = value.oid.substring(value.oid.lastIndexOf('.') + 1);
        working[workingCount].index = candidate;
        working[workingCount].name = value.text;
        stage = 1;
      } else if (stage == 1) {
        // Ethernet includes physical ports, bonds and bridges, but excludes loopback/tunnels.
        stage = result == 1 && value.num == 6 ? 2 : 0;
      } else {
        working[workingCount].up = result == 1 && value.num == 1;
        if (result != 1) {
          for (unsigned i = 0; i < count; i++)
            if (entries[i].index == candidate) working[workingCount].up = entries[i].up;
        }
        workingCount++;
        // Publish usable ports before the potentially long virtual-interface walk ends.
        unsigned n = 0;
        while (n < count && entries[n].index != candidate) n++;
        if (n < 16) {
          entries[n] = working[workingCount - 1];
          if (n == count) count++;
        }
        stage = 0;
        const auto &portEntry = working[workingCount - 1];
        if (bootstrap && portEntry.up &&
            (portEntry.name.startsWith("eth") || portEntry.name.startsWith("en"))) {
          // One usable physical port is sufficient to start rates. Resume the
          // remaining scan later rather than walking every virtual port at boot.
          bootstrap = false; paused = requested = true; running = false;
          completed = millis(); retryWait = Sampling::InterfacePeriod; return;
        }
      }
    }
    if (!client.busy())
      client.query(
          ip, port, community,
          stage == 0 ? walk : String("1.3.6.1.2.1.2.2.1.") + (stage == 1 ? "3." : "8.") + candidate,
          stage == 0);
  }
};
