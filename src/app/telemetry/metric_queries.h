#pragma once
#include "snmp/oid_catalog.h"
#include "snmp/device_profile.h"
#include "snmp/capability_plan.h"
#include "snmp/adapter_policy.h"
#include "snmp/identity_label.h"
#include "snmp/temperature_policy.h"
namespace Telemetry {
CapabilityPlan::Plan capabilities;
DeviceProfile::Match deviceProfile;
DeviceProfile::Match appliedProfile;
String systemDescription, systemObjectId, systemHostName;
bool identityNameFallback=false;
String sensorTemperatureOid, sensorCandidateOid;
bool identityReady=false, uptimeUnsupported=false, vendorModelUnsupported=false, verifiedSynologyModel=false;

inline bool probeSynologyModel() {
  return !vendorModelUnsupported && (DeviceProfile::synology(deviceProfile.system) ||
    deviceProfile.system==DeviceProfile::System::Linux ||
    DeviceProfile::subtree(systemObjectId.c_str(),"1.3.6.1.4.1.8072.3.2.10"));
}
inline void resetCapabilities() {
  capabilities.reset(); deviceProfile={};
  appliedProfile={};
  systemDescription=systemObjectId=systemHostName="";identityNameFallback=false;
  sensorTemperatureOid=sensorCandidateOid="";
  identityReady=uptimeUnsupported=vendorModelUnsupported=verifiedSynologyModel=false;
}
inline void beginCycle(unsigned group, Collector &job) {
  job.cursor=job.tableRows=0;job.walk="";
  for (auto &number:job.numbers) number=0;
  if (group==Cpu && capabilities.cpu==CapabilityPlan::Cpu::ProcessorTable) job.walk=SnmpOid::ProcessorLoad;
  if (group==Memory && capabilities.memory==CapabilityPlan::Memory::HostRam) job.walk=SnmpOid::StorageType;
  if (group==Temperature && capabilities.temperature==CapabilityPlan::Temperature::SensorTable && !sensorTemperatureOid.length()) {
    job.walk=SnmpOid::SensorTemperature;sensorCandidateOid="";
  }
}
inline bool supported(unsigned group) {
  return !(group==Cpu && capabilities.cpu==CapabilityPlan::Cpu::Unsupported) &&
    !(group==Memory && capabilities.memory==CapabilityPlan::Memory::Unsupported) &&
    !(group==Network && capabilities.network==CapabilityPlan::Network::Unsupported) &&
    !(group==Temperature && capabilities.temperature==CapabilityPlan::Temperature::Unsupported) &&
    !(group==Uptime && uptimeUnsupported);
}
inline unsigned fieldCount(unsigned group) {
  if (group==Identity) return 2+unsigned(identityNameFallback)+unsigned(probeSynologyModel());
  if (group==Memory) return capabilities.memory==CapabilityPlan::Memory::Available?2:4;
  if (group==Cpu) return capabilities.cpu==CapabilityPlan::Cpu::RawCounters?8:1;
  return group==Network?2:1;
}
inline String indexed(const char *root,const String &index) { return String(root)+'.'+index; }
inline String queryOid(unsigned group, const Collector &job, bool &next) {
  next=false;
  switch(group) {
    case Identity: {
      if(job.cursor==0) return SnmpOid::SystemDescription;
      if(job.cursor==1) return SnmpOid::SystemObjectId;
      if(job.cursor==2 && identityNameFallback) return SnmpOid::SystemName;
      return SnmpOid::Synology::Model;
    }
    case Cpu:
      if (capabilities.cpu==CapabilityPlan::Cpu::ProcessorTable) { next=true;return job.walk; }
      { const uint8_t rawColumns[]={50,51,52,53,54,56,61,64};return "1.3.6.1.4.1.2021.11."+String(rawColumns[job.cursor])+".0"; }
    case Memory:
      if (capabilities.memory==CapabilityPlan::Memory::HostRam) {
        if (!job.cursor) { next=true;return job.walk; }
        const char *columns[]={SnmpOid::StorageUnits,SnmpOid::StorageSize,SnmpOid::StorageUsed};
        return indexed(columns[job.cursor-1],job.walk);
      }
      if (capabilities.memory==CapabilityPlan::Memory::Available) return job.cursor?SnmpOid::MemoryAvailable:SnmpOid::MemoryTotal;
      { const char *columns[]={SnmpOid::MemoryTotal,SnmpOid::MemoryFree,SnmpOid::MemoryBuffers,SnmpOid::MemoryCached};return columns[job.cursor]; }
    case Network:
      return indexed(capabilities.network==CapabilityPlan::Network::Counter64?
        (job.cursor?SnmpOid::Transmit64:SnmpOid::Receive64):(job.cursor?SnmpOid::Transmit32:SnmpOid::Receive32),interfaceIndex);
    case Temperature:
      if (capabilities.temperature==CapabilityPlan::Temperature::Synology) return SnmpOid::Synology::Temperature;
      if (capabilities.temperature==CapabilityPlan::Temperature::CpuExtension) return SnmpOid::CpuTemperature;
      if(!job.walk.length() && sensorTemperatureOid.length()) return sensorTemperatureOid;
      if(job.cursor==1) return indexed(SnmpOid::SensorName,job.walk.substring(job.walk.lastIndexOf('.')+1));
      next=true;return job.walk.length()?job.walk:String(SnmpOid::SensorTemperature);
    default:return SnmpOid::HostUptime;
  }
}
inline void applyDiscoveredProfile() {
  const bool changed=!identityReady || appliedProfile.system!=deviceProfile.system || appliedProfile.evidence!=deviceProfile.evidence;
  appliedProfile=deviceProfile;identityReady=true;
  const auto adapter=SnmpAdapter::policy(deviceProfile);
  const auto wanted=adapter.vendorTemperature?CapabilityPlan::Temperature::Synology:CapabilityPlan::Temperature::SensorTable;
  if(changed && capabilities.temperature!=wanted) {
    auto &temperature=collectors[Temperature];
    temperature.client.cancel();temperature.active=false;temperature.schedule={};capabilities.temperature=wanted;
  }
  if(changed && adapter.freeAndCacheMemory && capabilities.memory==CapabilityPlan::Memory::Available) {
    auto &memory=collectors[Memory];
    memory.client.cancel();memory.active=false;memory.schedule={};capabilities.memory=CapabilityPlan::Memory::FreeAndCache;
  }
}
// 0: continue this snapshot, 1: coherent snapshot complete, -1: failed sample.
// Unsupported capabilities are remembered until the target/credentials reset.
inline int acceptValue(unsigned group, Collector &job, const Snmp::Value &value) {
  using namespace CapabilityPlan;
  const bool absent=unsupported(value.type);
  if (group==Identity) {
    if (!absent && (value.type!=(job.cursor==1?6:4) || !value.text.length())) return -1;
    if (!absent) {
      if (job.cursor==0) {
        systemDescription=value.text.substring(0,160);
        systemHostName=IdentityLabel::descriptionHost(systemDescription);
        identityNameFallback=!systemHostName.length();
      }
      else if (job.cursor==1) systemObjectId=value.text.substring(0,96);
      else if(job.cursor==2 && identityNameFallback) systemHostName=IdentityLabel::safeHost(value.text);
      else {
        metric.model=value.text.substring(0,64); verifiedSynologyModel=true;
        deviceProfile=DeviceProfile::classify(systemDescription.c_str(),systemObjectId.c_str(),true);
      }
    } else if(job.cursor==0) {systemDescription=systemHostName="";identityNameFallback=true;}
    else if (job.cursor>=2 && !(job.cursor==2 && identityNameFallback)) vendorModelUnsupported=true;
    if (job.cursor<2) deviceProfile=DeviceProfile::classify(systemDescription.c_str(),systemObjectId.c_str(),verifiedSynologyModel);
    // Standard enterprise evidence is sufficient to select an adapter. A
    // missing/slow model name must not delay temperature and RAM interpretation.
    if(job.cursor==1) applyDiscoveredProfile();
    if (++job.cursor<fieldCount(group)) return 0;
    applyDiscoveredProfile();
    return 1;
  }
  if (group==Cpu && capabilities.cpu==CapabilityPlan::Cpu::ProcessorTable) {
    if (!absent && value.oid==job.walk) return -1;
    if (absent || !DeviceProfile::subtree(value.oid.c_str(),SnmpOid::ProcessorLoad)) {
      if (job.tableRows) return 1;
      capabilities.cpu=CapabilityPlan::Cpu::Unsupported;return -1;
    }
    if (!numeric(value.type) || value.negative || value.num>100) return -1;
    job.numbers[0]+=value.num;job.walk=value.oid;
    return ++job.tableRows>=32?1:0;
  }
  if (group==Memory && capabilities.memory==CapabilityPlan::Memory::HostRam && !job.cursor) {
    if (!absent && value.oid==job.walk) return -1;
    if (absent || !DeviceProfile::subtree(value.oid.c_str(),SnmpOid::StorageType)) { capabilities.memory=CapabilityPlan::Memory::Unsupported;return -1; }
    if (value.type!=6) return -1;
    if (value.text==SnmpOid::StorageRam) { job.walk=value.oid.substring(value.oid.lastIndexOf('.')+1);job.cursor=1;return 0; }
    job.walk=value.oid;
    if (++job.tableRows>=64) { capabilities.memory=CapabilityPlan::Memory::Unsupported;return -1; }
    return 0;
  }
  if (group==Temperature && capabilities.temperature==CapabilityPlan::Temperature::SensorTable) {
    // Discovery uses a bounded walk once. Steady-state samples use one cached GET.
    auto finishSensors=[&]() {
      if(sensorCandidateOid.length()) {
        sensorTemperatureOid=sensorCandidateOid;sensorCandidateOid="";job.walk="";job.cursor=0;return 1;
      }
      capabilities.temperature=CapabilityPlan::Temperature::CpuExtension;beginCycle(group,job);return 0;
    };
    if(job.cursor==1) {
      const unsigned rank=!absent && value.type==4?TemperaturePolicy::rank(value.text.c_str()):10;
      if(rank && (rank>job.numbers[2] || (rank==job.numbers[2] && job.numbers[1]>job.numbers[0]))) {
        job.numbers[2]=rank;job.numbers[0]=job.numbers[1];sensorCandidateOid=job.walk;
      }
      job.cursor=0;
      if(rank>=100 || job.tableRows>=TemperaturePolicy::MaxRows) return finishSensors();
      return 0;
    }
    if(job.walk.length() || !sensorTemperatureOid.length()) {
      if(absent || !DeviceProfile::subtree(value.oid.c_str(),SnmpOid::SensorTemperature)) return finishSensors();
      if(value.oid==job.walk) return -1;
      job.walk=value.oid;++job.tableRows;
      if(numeric(value.type) && !value.negative && value.num<=TemperaturePolicy::MaxMillidegrees) {
        job.numbers[1]=value.num;job.cursor=1;return 0;
      }
      return job.tableRows>=TemperaturePolicy::MaxRows?finishSensors():0;
    }
    if(absent) {sensorTemperatureOid="";beginCycle(group,job);return 0;}
  }
  if (absent) {
    bool alternate=false;
    if (group==Cpu) { alternate=capabilities.fallbackCpu();cpuTicks.reset(); }
    else if (group==Memory) alternate=capabilities.fallbackMemory();
    else if (group==Network) { alternate=capabilities.fallbackNetwork();metric.counters=false; }
    else if (group==Temperature) {
      if (capabilities.temperature==CapabilityPlan::Temperature::Synology) { capabilities.temperature=CapabilityPlan::Temperature::SensorTable;alternate=true; }
      else if (capabilities.temperature==CapabilityPlan::Temperature::SensorTable) { capabilities.temperature=CapabilityPlan::Temperature::CpuExtension;alternate=true; }
      else capabilities.temperature=CapabilityPlan::Temperature::Unsupported;
    } else if (group==Uptime) uptimeUnsupported=true;
    if (alternate) { beginCycle(group,job);job.retry={};return 0; }
    return -1;
  }
  if (group==Temperature && capabilities.temperature==CapabilityPlan::Temperature::CpuExtension) {
    if(value.type!=4 || !value.text.length() || value.text.length()>6) return -1;
    uint32_t millidegrees=0;
    for(unsigned i=0;i<value.text.length();++i) {
      const char c=value.text[i];if(c<'0'||c>'9')return -1;
      millidegrees=millidegrees*10+c-'0';
    }
    if(millidegrees>150000)return -1;
    job.numbers[0]=millidegrees;return 1;
  }
  if (!numeric(value.type) || value.negative) return -1;
  if (group==Temperature && value.num>(capabilities.temperature==CapabilityPlan::Temperature::Synology?150u:150000u)) return -1;
  const unsigned field=group==Memory && capabilities.memory==CapabilityPlan::Memory::HostRam?job.cursor-1:job.cursor;
  job.numbers[field]=value.num;
  return ++job.cursor==fieldCount(group)?1:0;
}
}
