#pragma once
#include "snmp.h"
// On-demand, independent discovery: opening management never blocks live metrics.
class NetworkInterfaces {
  Snmp::Client client;
  String walk, candidate;
  unsigned stage = 0, steps = 0, workingCount = 0;
  bool requested = false, running = false;
  uint32_t completed = 0;
  const String root = "1.3.6.1.2.1.2.2.1.2";

public:
  struct Entry {
    String index, name;
    bool up = false;
  } entries[16], working[16];
  unsigned count = 0;
  void reset() {
    client.begin();
    requested = running = false;
    completed = 0;
    count = 0;
  }
  void request() {
    requested = true;
  }
  void update(IPAddress ip, unsigned port, const String &community) {
    if (!running) {
      if (!requested || (completed && millis() - completed < 60000))
        return;
      requested = false;
      running = true;
      walk = root;
      workingCount = steps = stage = 0;
    }
    Snmp::Value value;
    int result = client.poll(value);
    if (result) {
      if (stage == 0) {
        if (result != 1 || !value.oid.startsWith(root + ".") || ++steps > 64 ||
            workingCount == 16) {
          if (workingCount) {
            count = workingCount;
            for (unsigned i = 0; i < count; i++)
              entries[i] = working[i];
          }
          running = false;
          completed = millis();
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
        workingCount++;
        stage = 0;
      }
    }
    if (!client.busy())
      client.query(
          ip, port, community,
          stage == 0 ? walk : String("1.3.6.1.2.1.2.2.1.") + (stage == 1 ? "3." : "8.") + candidate,
          stage == 0);
  }
};
