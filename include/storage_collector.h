#pragma once
#include "snmp.h"
#include "dashboard.h"
#include "resource_metrics.h"
#include "snmp/oid_catalog.h"
#include "snmp/storage_policy.h"
// Independent SNMP socket keeps disk/volume discovery from delaying live CPU/network queries.
class StorageCollector {
  Snmp::Client client;
  Snmp::Client volumeClient, temperatureClient;
  uint32_t temperatureAttempt = 0;
  unsigned temperatureEntry = 0;
  bool temperatureRunning = false;
  bool volumeRunning = false, volumeBootstrapDone = false, diskBootstrapDone = false;
  unsigned volumeEntry = 0;
  uint32_t volumeAttempt = 0, volumeAt[8] = {};
  unsigned volumeMask[8] = {};
  IPAddress ip;
  String community, walk;
  String diskResume, volumeResume;
  bool volumeProbeDone=false, genericInventoryDone=false;
  unsigned stage = 0, steps = 0, entry = 0, field = 0;
  uint32_t at = 0;
  bool running = false, cached = false;
  bool discoveryFailed = false, paused = false;
  uint32_t discoveryAt = 0;
  uint32_t diskFieldAt[12][3] = {};
  const String diskRoot = SnmpOid::Synology::Disks, volumeRoot = SnmpOid::StorageName;
  bool volumeName(const String &s) {
    // Mounted data volumes only; never infer USB from /mnt or disk names.
    if (s == "/" || s.startsWith("/mnt/") || s.startsWith("/media/") ||
        s.startsWith("/run/media/") || s.startsWith("/tmp/mnt/") || s.startsWith("/www")) return true;
    if (s.startsWith("/volumeUSB")) {
      unsigned end = 10;
      while (end < s.length() && s[end] >= '0' && s[end] <= '9') ++end;
      return end > 10 && (end == s.length() || s[end] == '/');
    }
    if (s.startsWith("/vol") && !s.startsWith("/volume")) return true;
    if (!s.startsWith("/volume") || s.length() < 8) return false;
    for (unsigned i = 7; i < s.length(); i++)
      if (s[i] < '0' || s[i] > '9') return false;
    return true;
  }

  static bool wholeDiskName(const String &name) {
    return StoragePolicy::wholeDisk(name.c_str());
  }
  void releaseWorking() {
    // Published rows own the cache. Discovery copies are needed only while a
    // walk is active/paused; keeping both sets for hours duplicates their text.
    workingDiskCount = workingVolumeCount = 0;
    for (auto &disk : workingDisks) disk = Dashboard::Disk();
    for (auto &volume : workingVolumes) volume = Dashboard::Volume();
    community = String(); walk = String(); diskResume = String(); volumeResume = String();
  }
  static void clearField(Dashboard::Disk &disk, unsigned field) {
    if (field == 0) disk.status = -1;
    else if (field == 1) disk.temp = -1;
    else disk.health = -1;
  }

public:
  uint32_t requestsSent() const {return client.sent+volumeClient.sent+temperatureClient.sent;}
  bool discoveryActive() const {return running || paused;}
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
    volumeAttempt = 0; volumeBootstrapDone = diskBootstrapDone = false;
    for (unsigned i = 0; i < 8; i++) volumeAt[i] = volumeMask[i] = 0;
    running = paused = false;
    at = discoveryAt = diskUpdated = volumeUpdated = 0;
    diskCount = volumeCount = 0;
    workingDiskCount = workingVolumeCount = 0;
    cached = discoveryFailed = online = false;
    community = walk = "";
    diskResume=volumeResume="";volumeProbeDone=false;genericInventoryDone=false;
    for (auto &d : disks) d = Dashboard::Disk();
    for (auto &d : workingDisks) d = Dashboard::Disk();
    for (auto &v : volumes) v = Dashboard::Volume();
    for (auto &v : workingVolumes) v = Dashboard::Volume();
    for (auto &fields : diskFieldAt)
      for (auto &stamp : fields)
        stamp = 0;
  }
  void suspend() {
    client.cancel(); volumeClient.cancel(); temperatureClient.cancel();
    running = paused = volumeRunning = temperatureRunning = false;
  }
  void rediscover() {
    // Keep visible cached readings while the rate-limited discovery runs again.
    client.cancel(); volumeClient.cancel();
    running = paused = volumeRunning = false;
    discoveryAt = at = 0; volumeBootstrapDone = diskBootstrapDone = false;
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
  void update(IPAddress host, unsigned port, const String &secret, bool preferDisks = false, bool includeRoot = true, bool vendorDisks = true, bool preferVolumes = false) {
    uint32_t now = millis();
    expire(now);
    if (vendorDisks) refreshTemperatures(host, port, secret, now);
    refreshVolumes(host, port, secret, now);
    if (!running) {
      bool cold = paused && ((stage == 0 && !diskCount) || (stage == 1 && !volumeCount));
      uint32_t cooldown = discoveryFailed ? (cold ? Sampling::DiscoveryRetry : Sampling::RetryCeiling) :
                          (!diskCount && volumeCount && discoveryAt ? Sampling::TableTtl : Sampling::DiskPeriod);
      if (StoragePolicy::needsVendorBootstrap(vendorDisks,diskBootstrapDone,volumeBootstrapDone)) cooldown = 0;
      if (at && now - at < cooldown) return;
      if (paused) { running = true; paused = false; discoveryFailed = false; }
      else {
        diskResume=volumeResume="";volumeProbeDone=false;genericInventoryDone=false;
        ip = host;
        community = secret;
        stage = 0;
        steps = 0;
        walk = diskRoot + ".2";
        // The volume startup page does not wait for the complete disk walk.
        if (!vendorDisks || (preferVolumes && !preferDisks && !volumeBootstrapDone)) { stage = 1; walk = volumeRoot; }
        running = true;
        cached = diskCount && volumeCount && discoveryAt && now - discoveryAt < Sampling::TableTtl;
        discoveryFailed = false;
        if (cached && !vendorDisks) { running=false; at=now; return; }
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
      if (result==1 && stage==1 && v.oid.startsWith(volumeRoot+".") && (!v.valid || v.type!=4)) {
        paused=discoveryFailed=true;running=false;at=now;return;
      }
      if (result != 1 && (stage < 2 || stage == 4)) {
        // Retry the failed GETNEXT from its last successful cursor. Restarting
        // a long table at its root after every lost reply may never finish.
        paused = discoveryFailed = true;
        running = false; at = now;
        return;
      }
      if (stage == 5 || stage == 7) {
        const String root = stage == 5 ? "1.3.6.1.2.1.25.3.6.1.2" : "1.3.6.1.4.1.2021.13.15.1.1.2";
        if (result == 1 && v.valid && v.oid.startsWith(root+".") && steps++ < 64 && workingDiskCount < 12) {
          walk=v.oid;
          const String index=v.oid.substring(v.oid.lastIndexOf('.')+1);
          if (StoragePolicy::index(index.c_str()) && (stage==5 ? StoragePolicy::numeric(v.type,v.valid,v.negative) : v.type==4 && wholeDiskName(v.text))) {
            auto &disk=workingDisks[workingDiskCount++]; disk=Dashboard::Disk(); disk.index=index;
            disk.name=stage==7?v.text.substring(0,24):String("Disk ")+index;
            disk.model="--";
            entry=workingDiskCount-1;field=0;
            if(stage==5) stage=6;
            else {disks[entry]=disk;diskCount=workingDiskCount;diskUpdated=now;}
          }
        } else if(stage==5 && !workingDiskCount) {stage=7;steps=0;walk="1.3.6.1.4.1.2021.13.15.1.1.2";}
        else {genericInventoryDone=true;diskBootstrapDone=true;stage=2;entry=workingVolumeCount;field=0;}
      } else if(stage==6) {
        auto &disk=workingDisks[entry];
        if(result==1 && v.valid) {
          if(field==0 && v.type==4) {
            disk.model=v.text.substring(0,64);
            int start=v.text.indexOf("/dev/");
            if(start>=0) {disk.name=v.text.substring(start+5);int end=disk.name.indexOf(')');if(end>=0)disk.name=disk.name.substring(0,end);}
            // HOST-RESOURCES includes RAID/LVM devices. They belong to volume
            // capacity, not physical disk inventory. Skip their capacity GET.
            if((start>=0 && !wholeDiskName(disk.name)) || v.text.startsWith("RAID ") || v.text.startsWith("LVM ")) {
              disk=Dashboard::Disk();--workingDiskCount;stage=5;field=0;
              return;
            }
          } else if(field==1 && StoragePolicy::numeric(v.type,v.valid,v.negative) && v.num>0) {
            // hrDiskStorageCapacity is KiB, not bytes. It conveys no SMART status.
            disk.model=String(double(v.num)/1048576.0,1)+" GiB";
          }
        }
        if(++field==2) {disks[entry]=disk;diskCount=workingDiskCount;diskUpdated=now;stage=5;}
      } else if (stage == 3) {
        const unsigned column=field==0?5:field==1?6:13;
        const bool accepted=result == 1 && entry < workingDiskCount && StoragePolicy::diskValue(column,v.type,v.valid,v.negative,v.num);
        if (accepted) {
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
        if (entry < diskCount) { disks[entry] = workingDisks[entry]; if(accepted) diskUpdated = now; }
        if (++field == 1) field = 2; // Temperature has its own five-minute job.
        if (field == 3) { field = 0; entry++; }
        if (entry >= workingDiskCount) {
          for (unsigned i = 0; i < diskCount; i++)
            disks[i] = workingDisks[i];
          // Cached disks and volume use have independent sockets and deadlines.
          running = false;
          at = now;
          return;
        }
      } else if (stage == 0) {
        if (result == 1 && v.valid && v.oid.startsWith(diskRoot + ".") && steps++ < 180) {
          String tail = v.oid.substring(diskRoot.length() + 1);
          int dot = tail.indexOf('.');
          unsigned column = tail.substring(0, dot).toInt();
          String index = tail.substring(dot + 1);
          if (dot<1 || dot>2 || !StoragePolicy::index(index.c_str())) { walk=v.oid; return; }
          if (column > 13) {
            diskCount = workingDiskCount;
            for (unsigned i = 0; i < diskCount; i++)
              disks[i] = workingDisks[i];
            diskUpdated = now; diskBootstrapDone = true;
            if (volumeCount && discoveryAt && now - discoveryAt < Sampling::TableTtl) { running = false; at = now; releaseWorking(); return; }
            stage = volumeBootstrapDone && volumeProbeDone ? 2 : 1;
            entry=workingVolumeCount;field=0;
            steps = 0;
            walk = volumeResume.length()?volumeResume:volumeRoot;volumeResume="";
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
              bool accepted=StoragePolicy::diskValue(column,v.type,v.valid,v.negative,v.num);
              if (column == 2 && v.type==4)
                d.name = v.text.substring(0,64);
              else if (column == 3 && v.type==4)
                d.model = v.text.substring(0,64);
              else if (column == 5 && accepted)
                d.status = v.num;
              else if (column == 6 && accepted)
                d.temp = int(v.num);
              else if (column == 13 && accepted)
                d.health = int(v.num);
              if (accepted)
                diskFieldAt[n][column == 5 ? 0 : column == 6 ? 1 : 2] = now;
              // First discovery publishes useful rows before optional fields finish.
              if (!diskBootstrapDone && d.name.length() && (d.status>=0 || d.temp>=0)) {
                disks[n]=d; if(diskCount<=n) diskCount=n+1; diskUpdated=now;
              }
            }
            walk = column > 6 && column < 13 ? diskRoot + ".13" : v.oid;
            // Yield the disk walk after its first status to obtain one
            // volume, then resume this exact cursor without repeating rows.
            if(column==5 && StoragePolicy::diskValue(column,v.type,v.valid,v.negative,v.num) &&
               !volumeProbeDone && !volumeBootstrapDone && !preferDisks) {
              diskResume=walk;walk=volumeRoot;stage=1;steps=0;volumeProbeDone=true;
            }
          }
        } else {
          diskCount = workingDiskCount;
          for (unsigned i = 0; i < diskCount; i++)
            disks[i] = workingDisks[i];
          diskUpdated = now; diskBootstrapDone = true;
          if (volumeCount && discoveryAt && now - discoveryAt < Sampling::TableTtl) { running = false; at = now; releaseWorking(); return; }
          stage = volumeBootstrapDone && volumeProbeDone ? 2 : 1;
          entry=workingVolumeCount;field=0;
          steps = 0;
          walk = volumeResume.length()?volumeResume:volumeRoot;volumeResume="";
        }
      } else if (stage == 1) {
        if (result == 1 && v.valid && v.type==4 && v.oid.startsWith(volumeRoot + ".") && steps++ < 128) {
          bool duplicateRoot = false;
          if (v.text == "/") for (unsigned i = 0; i < workingVolumeCount; ++i)
            if (workingVolumes[i].name == "/overlay") duplicateRoot = true;
          if (!duplicateRoot && StoragePolicy::candidate(v.text.c_str(),includeRoot,vendorDisks) && workingVolumeCount < 8) {
            auto &d = workingVolumes[workingVolumeCount++];
            d = Dashboard::Volume();
            d.name = v.text.substring(0,128);
            d.index = v.oid.substring(v.oid.lastIndexOf('.') + 1);
            // Fetch this volume immediately, before scanning remaining names.
            stage = 4; entry = workingVolumeCount - 1; field = 0;
          }
          walk = v.oid;
        } else {
          if(diskResume.length()) {
            volumeBootstrapDone=true;stage=0;walk=diskResume;diskResume="";steps=0;
          } else {stage = 2;entry = workingVolumeCount; field = 0;}
        }
      } else if (stage == 4) {
        // Verify real disk-backed storage instead of assuming a mount path is a disk.
        const String kind = v.text;
        bool accepted = result == 1 && v.type == 6 && StoragePolicy::diskBacked(kind.c_str());
        if (result == 1 && v.type >= 0x80) accepted = volumeName(workingVolumes[entry].name);
        if (accepted) { stage = 2; field = 0; }
        else { --workingVolumeCount; stage = 1; }
      } else {
        if (result == 1 && StoragePolicy::numeric(v.type,v.valid,v.negative) && entry < workingVolumeCount) {
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
          volume.valid = volumeMask[entry] == 7 && StoragePolicy::capacity(volume.units,volume.size,volume.used);
          if (volume.valid) {
            volumes[entry] = volume; volumeAt[entry] = now;
            if (volumeCount <= entry) volumeCount = entry + 1;
            volumeUpdated = now;
          }
          field = 0;
          entry++;
          if(volume.valid && diskResume.length()) {
            volumeResume=walk;walk=diskResume;diskResume="";stage=0;steps=0;
          } else stage = 1; // Resume names; completed volumes are not queried twice.
        }
      }
    }
    // The display stores eight volumes; stop walking names once that bound is full.
    if (stage==1 && workingVolumeCount>=8) {stage=2;entry=workingVolumeCount;field=0;}
    if(stage==2 && entry>=workingVolumeCount && !vendorDisks && !genericInventoryDone) {
      stage=5;steps=0;walk="1.3.6.1.2.1.25.3.6.1.2";
    }
    if (stage == 2 && entry >= workingVolumeCount) {
      diskCount = workingDiskCount;
      for (unsigned i = 0; i < diskCount; i++)
        disks[i] = workingDisks[i];
      uint32_t stamps[8] = {};
      for (unsigned i = 0; i < workingVolumeCount; i++) {
        auto &volume = workingVolumes[i];
        volume.valid = volumeMask[i] == 7 && StoragePolicy::capacity(volume.units,volume.size,volume.used);
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
      // A Linux host without the Synology disk table is still a completed scan.
      // Retain its empty disk result instead of continuously walking volumes.
      at = now;
      releaseWorking();
      return;
    }
    if (!client.busy()) {
      if(stage==5 || stage==7) client.query(ip,port,community,walk,true);
      else if(stage==6) client.query(ip,port,community,
        (field==0?String("1.3.6.1.2.1.25.3.2.1.3."):String("1.3.6.1.2.1.25.3.6.1.4."))+workingDisks[entry].index);
      else if (stage == 3) {
        unsigned column = field == 0 ? 5 : field == 1 ? 6 : 13;
        client.query(ip, port, community,
                     diskRoot + "." + String(column) + "." + workingDisks[entry].index);
      } else if (stage < 2)
        client.query(ip, port, community, walk, true);
      else if (stage == 4)
        client.query(ip, port, community, "1.3.6.1.2.1.25.2.3.1.2." + workingVolumes[entry].index);
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
      if (result == 1 && StoragePolicy::diskValue(6,value.type,value.valid,value.negative,value.num)) {
        disks[temperatureEntry].temp = value.num;
        if (temperatureEntry < workingDiskCount) workingDisks[temperatureEntry].temp = value.num;
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
      if (result == 1 && StoragePolicy::numeric(v.type,v.valid,v.negative) && StoragePolicy::capacity(volume.units,volume.size,v.num)) {
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
