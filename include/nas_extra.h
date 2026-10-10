#pragma once
#include "snmp.h"
#include "dashboard.h"
// Independent SNMP socket keeps disk/volume discovery from delaying live CPU/network queries.
class NasExtra {
  Snmp::Client client;
  IPAddress ip;
  String community, walk;
  unsigned stage = 0, steps = 0, entry = 0, field = 0;
  uint32_t at = 0;
  bool running = false, cached = false;
  uint32_t discoveryAt = 0;
  const String diskRoot = "1.3.6.1.4.1.6574.2.1.1", volumeRoot = "1.3.6.1.2.1.25.2.3.1.3";
  bool volumeName(const String &s) {
    if (!s.startsWith("/volume") || s.length() < 8)
      return false;
    for (unsigned i = 7; i < s.length(); i++)
      if (s[i] < '0' || s[i] > '9')
        return false;
    return true;
  }

public:
  Dashboard::Disk workingDisks[12], disks[12];
  unsigned workingDiskCount = 0, diskCount = 0;
  Dashboard::Volume workingVolumes[8], volumes[8];
  unsigned workingVolumeCount = 0, volumeCount = 0;
  bool online = false;
  uint32_t diskUpdated = 0, volumeUpdated = 0;
  void begin() {
    client.begin();
  }
  void reset() {
    client.cancel();
    running = false;
    at = discoveryAt = diskUpdated = volumeUpdated = 0;
    diskCount = volumeCount = 0;
  }
  void update(IPAddress host, unsigned port, const String &secret) {
    uint32_t now = millis();
    if (!running) {
      if (at && now - at < 5000)
        return;
      ip = host;
      community = secret;
      stage = 0;
      steps = 0;
      walk = diskRoot;
      running = true;
      cached = diskCount && volumeCount && discoveryAt && now - discoveryAt < 300000;
      if (cached) {
        stage = 3;
        entry = field = 0;
        workingDiskCount = diskCount;
        workingVolumeCount = volumeCount;
        for (unsigned i = 0; i < diskCount; i++)
          workingDisks[i] = disks[i];
        for (unsigned i = 0; i < volumeCount; i++)
          workingVolumes[i] = volumes[i];
      } else {
        discoveryAt = now;
        workingDiskCount = workingVolumeCount = 0;
      }
    }
    Snmp::Value v;
    int result = client.poll(v);
    if (result) {
      if (stage == 3) {
        if (result == 1 && entry < workingDiskCount) {
          auto &d = workingDisks[entry];
          if (field == 0)
            d.status = v.num;
          else if (field == 1)
            d.temp = v.negative ? -1 : int(v.num);
          else
            d.health = v.negative ? -1 : int(v.num);
        } else if (entry < workingDiskCount) {
          auto &d = workingDisks[entry];
          if (field == 0)
            d.status = -1;
          else if (field == 1)
            d.temp = -1;
          else
            d.health = -1;
        }
        if (++field == 3) {
          field = 0;
          entry++;
        }
        if (entry >= workingDiskCount) {
          for (unsigned i = 0; i < diskCount; i++)
            disks[i] = workingDisks[i];
          diskUpdated = now;
          stage = 2;
          entry = 0;
          field = 2;
        }
      } else if (stage == 0) {
        if (result == 1 && v.oid.startsWith(diskRoot + ".") && steps++ < 180) {
          String tail = v.oid.substring(diskRoot.length() + 1);
          int dot = tail.indexOf('.');
          unsigned column = tail.substring(0, dot).toInt();
          String index = tail.substring(dot + 1);
          if (column > 13) {
            diskCount = workingDiskCount;
            for (unsigned i = 0; i < diskCount; i++)
              disks[i] = workingDisks[i];
            diskUpdated = now;
            stage = 1;
            steps = 0;
            walk = volumeRoot;
          } else {
            unsigned n = 0;
            while (n < workingDiskCount && workingDisks[n].index != index)
              n++;
            if (n == workingDiskCount && workingDiskCount < 12) {
              workingDisks[workingDiskCount] = Dashboard::Disk();
              workingDisks[workingDiskCount++].index = index;
            }
            if (n < workingDiskCount) {
              auto &d = workingDisks[n];
              if (column == 2)
                d.name = v.text;
              else if (column == 3)
                d.model = v.text;
              else if (column == 5)
                d.status = v.num;
              else if (column == 6)
                d.temp = v.num;
              else if (column == 13)
                d.health = v.num;
            }
            walk = column > 6 && column < 13 ? diskRoot + ".13" : v.oid;
          }
        } else {
          diskCount = workingDiskCount;
          for (unsigned i = 0; i < diskCount; i++)
            disks[i] = workingDisks[i];
          diskUpdated = now;
          stage = 1;
          steps = 0;
          walk = volumeRoot;
        }
      } else if (stage == 1) {
        if (result == 1 && v.oid.startsWith(volumeRoot + ".") && steps++ < 128) {
          if (volumeName(v.text) && workingVolumeCount < 8) {
            auto &d = workingVolumes[workingVolumeCount++];
            d = Dashboard::Volume();
            d.name = v.text;
            d.index = v.oid.substring(v.oid.lastIndexOf('.') + 1);
          }
          walk = v.oid;
        } else {
          stage = 2;
          entry = field = 0;
        }
      } else {
        if (result == 1 && entry < workingVolumeCount) {
          auto &d = workingVolumes[entry];
          if (field == 0)
            d.units = v.num;
          else if (field == 1)
            d.size = v.num;
          else
            d.used = v.num;
        }
        if (++field == 3) {
          field = cached ? 2 : 0;
          entry++;
        }
      }
    }
    if (stage == 2 && entry >= workingVolumeCount) {
      diskCount = workingDiskCount;
      volumeCount = workingVolumeCount;
      for (unsigned i = 0; i < diskCount; i++)
        disks[i] = workingDisks[i];
      for (unsigned i = 0; i < volumeCount; i++)
        volumes[i] = workingVolumes[i];
      online = diskCount || volumeCount;
      volumeUpdated = now;
      running = false;
      at = now;
      return;
    }
    if (!client.busy()) {
      if (stage == 3) {
        unsigned column = field == 0 ? 5 : field == 1 ? 6 : 13;
        client.query(ip, port, community,
                     diskRoot + "." + String(column) + "." + workingDisks[entry].index);
      } else if (stage < 2)
        client.query(ip, port, community, walk, true);
      else
        client.query(ip, port, community,
                     "1.3.6.1.2.1.25.2.3.1." + String(field + 4) + "." +
                         workingVolumes[entry].index);
    }
  }
};
