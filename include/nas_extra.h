#pragma once
#include "snmp.h"
#include "dashboard.h"
#include "resource_metrics.h"
// Independent SNMP socket keeps disk/volume discovery from delaying live CPU/network queries.
class NasExtra {
  Snmp::Client client;
  Snmp::Client volumeClient, temperatureClient;
  uint32_t temperatureAttempt = 0;
  unsigned temperatureEntry = 0;
  bool temperatureRunning = false;
  bool volumeRunning = false, volumeBootstrapDone = false;
  unsigned volumeEntry = 0;
  uint32_t volumeAttempt = 0, volumeAt[8] = {};
  unsigned volumeMask[8] = {};
  IPAddress ip;
  String community, walk;
  unsigned stage = 0, steps = 0, entry = 0, field = 0;
  uint32_t at = 0;
  bool running = false, cached = false;
  bool discoveryFailed = false, paused = false;
  uint32_t discoveryAt = 0;
  uint32_t diskFieldAt[12][3] = {};
  const String diskRoot = "1.3.6.1.4.1.6574.2.1.1", volumeRoot = "1.3.6.1.2.1.25.2.3.1.3";
  bool volumeName(const String &s) {
    if (!s.startsWith("/volume") || s.length() < 8)
      return false;
    for (unsigned i = 7; i < s.length(); i++)
      if (s[i] < '0' || s[i] > '9')
        return false;
    return true;
  }

  static void clearField(Dashboard::Disk &disk, unsigned field) {
    if (field == 0) disk.status = -1;
    else if (field == 1) disk.temp = -1;
    else disk.health = -1;
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
    volumeClient.begin();
    temperatureClient.begin();
  }
  void reset() {
    client.cancel();
    volumeClient.cancel();
    temperatureClient.cancel();
    temperatureRunning = false; temperatureAttempt = 0;
    volumeRunning = false;
    volumeAttempt = 0; volumeBootstrapDone = false;
    for (unsigned i = 0; i < 8; i++) volumeAt[i] = volumeMask[i] = 0;
    running = paused = false;
    at = discoveryAt = diskUpdated = volumeUpdated = 0;
    diskCount = volumeCount = 0;
    for (auto &fields : diskFieldAt)
      for (auto &stamp : fields)
        stamp = 0;
  }
  void expire(uint32_t now) {
    // Expiration is independent of pending disk requests.
    for (unsigned i = 0; i < diskCount; i++) {
      for (unsigned f = 0; f < 3; f++) {
        if (Resources::expireSample(now, diskFieldAt[i][f], f == 1 ? Sampling::DiskTemperatureTtl : Sampling::DiskTtl)) {
          clearField(disks[i], f);
          for (unsigned j = 0; j < workingDiskCount; j++)
            if (workingDisks[j].index == disks[i].index) clearField(workingDisks[j], f);
          diskUpdated = now;
        }
      }
    }
    for (unsigned i = 0; i < volumeCount; i++) {
      if (volumes[i].valid && (!volumeAt[i] || now - volumeAt[i] > Sampling::VolumeTtl)) {
        volumes[i].valid = false; volumeUpdated = now;
      }
    }
  }
  void update(IPAddress host, unsigned port, const String &secret, bool preferDisks = false) {
    uint32_t now = millis();
    expire(now);
    refreshTemperatures(host, port, secret, now);
    refreshVolumes(host, port, secret, now);
    if (!running) {
      bool cold = paused && ((stage == 0 && !diskCount) || (stage == 1 && !volumeCount));
      uint32_t cooldown = discoveryFailed ? (cold ? Sampling::DiscoveryRetry : Sampling::RetryCeiling) : Sampling::DiskPeriod;
      if (at && now - at < cooldown) return;
      if (paused) { running = true; paused = false; discoveryFailed = false; }
      else {
        ip = host;
        community = secret;
        stage = 0;
        steps = 0;
        walk = diskRoot;
        // The volume startup page does not wait for the complete disk walk.
        if (!preferDisks && !volumeBootstrapDone && !diskCount && !volumeCount) { stage = 1; walk = volumeRoot; }
        running = true;
        cached = diskCount && volumeCount && discoveryAt && now - discoveryAt < Sampling::TableTtl;
        discoveryFailed = false;
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
          temperatureClient.cancel(); temperatureRunning = false;
          workingDiskCount = workingVolumeCount = 0;
          for (auto &mask : volumeMask) mask = 0;
        }
      }
    }
    Snmp::Value v;
    int result = client.poll(v);
    if (result) {
      if (result != 1 && stage < 2) {
        // Retry the failed GETNEXT from its last successful cursor. Restarting
        // a long table at its root after every lost reply may never finish.
        paused = discoveryFailed = true;
        running = false; at = now;
        return;
      }
      if (stage == 3) {
        if (result == 1 && entry < workingDiskCount) {
          diskFieldAt[entry][field] = now;
          auto &d = workingDisks[entry];
          if (field == 0)
            d.status = v.num;
          else if (field == 1)
            d.temp = v.negative ? -1 : int(v.num);
          else
            d.health = v.negative ? -1 : int(v.num);
        } else if (entry < workingDiskCount &&
                   (!diskFieldAt[entry][field] ||
                    Resources::expireSample(now, diskFieldAt[entry][field], Sampling::DiskTtl))) {
          auto &d = workingDisks[entry];
          if (field == 0)
            d.status = -1;
          else if (field == 1)
            d.temp = -1;
          else
            d.health = -1;
        }
        // Publish a completed field immediately; other disks may still time out.
        if (entry < diskCount) { disks[entry] = workingDisks[entry]; diskUpdated = now; }
        if (++field == 1) field = 2; // Temperature has its own five-minute job.
        if (field == 3) { field = 0; entry++; }
        if (entry >= workingDiskCount) {
          for (unsigned i = 0; i < diskCount; i++)
            disks[i] = workingDisks[i];
          diskUpdated = now;
          // Cached disks and volume use have independent sockets and deadlines.
          running = false;
          at = now;
          return;
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
            if (volumeCount && discoveryAt && now - discoveryAt < Sampling::TableTtl) { running = false; at = now; return; }
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
                d.temp = v.negative ? -1 : int(v.num);
              else if (column == 13)
                d.health = v.negative ? -1 : int(v.num);
              if (column == 5 || column == 6 || column == 13)
                diskFieldAt[n][column == 5 ? 0 : column == 6 ? 1 : 2] = now;
            }
            walk = column > 6 && column < 13 ? diskRoot + ".13" : v.oid;
          }
        } else {
          diskCount = workingDiskCount;
          for (unsigned i = 0; i < diskCount; i++)
            disks[i] = workingDisks[i];
          diskUpdated = now;
          if (volumeCount && discoveryAt && now - discoveryAt < Sampling::TableTtl) { running = false; at = now; return; }
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
            // Fetch this volume immediately, before scanning remaining names.
            stage = 2; entry = workingVolumeCount - 1; field = 0;
          }
          walk = v.oid;
        } else {
          stage = 2;
          entry = workingVolumeCount; field = 0;
        }
      } else {
        if (result == 1 && !v.negative && entry < workingVolumeCount) {
          auto &d = workingVolumes[entry];
          if (field == 0)
            d.units = v.num;
          else if (field == 1)
            d.size = v.num;
          else
            d.used = v.num;
          volumeMask[entry] |= 1u << field;
        }
        if (++field == 3) {
          auto &volume = workingVolumes[entry];
          volume.valid = volumeMask[entry] == 7 && volume.size && volume.used <= volume.size;
          if (volume.valid) {
            volumes[entry] = volume; volumeAt[entry] = now;
            if (volumeCount <= entry) volumeCount = entry + 1;
            volumeUpdated = now;
          }
          field = 0;
          entry++;
          stage = 1; // Resume the name walk; no completed volume is queried twice.
        }
      }
    }
    if (stage == 2 && entry >= workingVolumeCount) {
      diskCount = workingDiskCount;
      for (unsigned i = 0; i < diskCount; i++)
        disks[i] = workingDisks[i];
      uint32_t stamps[8] = {};
      for (unsigned i = 0; i < workingVolumeCount; i++) {
        auto &volume = workingVolumes[i];
        volume.valid = volumeMask[i] == 7 && volume.size && volume.used <= volume.size;
        if (volume.valid) stamps[i] = now;
        else {
          discoveryFailed = true;
          for (unsigned j = 0; j < volumeCount; j++) {
            if (volumes[j].index == volume.index) {
              volume = volumes[j]; stamps[i] = volumeAt[j]; break;
            }
          }
        }
      }
      volumeBootstrapDone = true;
      volumeClient.cancel(); volumeRunning = false; volumeAttempt = now;
      volumeCount = workingVolumeCount;
      for (unsigned i = 0; i < volumeCount; i++) {
        volumes[i] = workingVolumes[i]; volumeAt[i] = stamps[i];
      }
      discoveryAt = discoveryFailed ? 0 : now;
      temperatureAttempt = now;
      online = diskCount || volumeCount;
      volumeUpdated = now;
      running = false;
      at = !diskCount && volumeCount ? 0 : now;
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
  void refreshTemperatures(IPAddress host, unsigned port, const String &secret, uint32_t now) {
    if (!diskCount || !discoveryAt || (running && !cached)) return;
    if (!temperatureRunning) {
      if (temperatureAttempt && now - temperatureAttempt < Sampling::DiskTemperaturePeriod) return;
      temperatureRunning = true; temperatureEntry = 0;
    }
    Snmp::Value value;
    int result = temperatureClient.poll(value);
    if (result) {
      if (result == 1 && !value.negative) {
        disks[temperatureEntry].temp = value.num;
        workingDisks[temperatureEntry].temp = value.num;
        diskFieldAt[temperatureEntry][1] = now; diskUpdated = now;
      }
      if (++temperatureEntry >= diskCount) {
        temperatureRunning = false; temperatureAttempt = now; return;
      }
    }
    if (!temperatureClient.busy())
      temperatureClient.query(host, port, secret, diskRoot + ".6." + disks[temperatureEntry].index);
  }
  void refreshVolumes(IPAddress host, unsigned port, const String &secret, uint32_t now) {
    // A metadata walk also samples volume use; avoid a duplicate refresh.
    if (!volumeCount || !discoveryAt || now - discoveryAt >= Sampling::TableTtl ||
        (running && !cached)) return;
    if (!volumeRunning) {
      if (volumeAttempt && now - volumeAttempt < Sampling::VolumePeriod) return;
      volumeRunning = true; volumeEntry = 0;
    }
    Snmp::Value v;
    int result = volumeClient.poll(v);
    if (result) {
      auto &volume = volumes[volumeEntry];
      if (result == 1 && !v.negative && volume.size && v.num <= volume.size) {
        volume.used = v.num; volume.valid = true; volumeAt[volumeEntry] = now;
        volumeUpdated = now;
      }
      if (++volumeEntry >= volumeCount) {
        volumeRunning = false; volumeAttempt = now;
        return;
      }
    }
    if (!volumeClient.busy())
      volumeClient.query(host, port, secret,
                         "1.3.6.1.2.1.25.2.3.1.6." + volumes[volumeEntry].index);
  }
};
