#pragma once
// Numeric OIDs are protocol data, not presentation text. Keep each root here
// once; adapters and collectors refer to names instead of magic field numbers.
namespace SnmpOid {
constexpr const char *SystemDescription = "1.3.6.1.2.1.1.1.0";
constexpr const char *SystemObjectId = "1.3.6.1.2.1.1.2.0";
constexpr const char *SystemName = "1.3.6.1.2.1.1.5.0";
constexpr const char *HostUptime = "1.3.6.1.2.1.25.1.1.0";
constexpr const char *ProcessorLoad = "1.3.6.1.2.1.25.3.3.1.2";
constexpr const char *MemoryTotal = "1.3.6.1.4.1.2021.4.5.0";
constexpr const char *MemoryAvailable = "1.3.6.1.4.1.2021.4.27.0";
constexpr const char *MemoryFree = "1.3.6.1.4.1.2021.4.6.0";
constexpr const char *MemoryBuffers = "1.3.6.1.4.1.2021.4.14.0";
constexpr const char *MemoryCached = "1.3.6.1.4.1.2021.4.15.0";
constexpr const char *Receive64 = "1.3.6.1.2.1.31.1.1.1.6";
constexpr const char *Transmit64 = "1.3.6.1.2.1.31.1.1.1.10";
constexpr const char *Receive32 = "1.3.6.1.2.1.2.2.1.10";
constexpr const char *Transmit32 = "1.3.6.1.2.1.2.2.1.16";
constexpr const char *StorageType = "1.3.6.1.2.1.25.2.3.1.2";
constexpr const char *StorageName = "1.3.6.1.2.1.25.2.3.1.3";
constexpr const char *StorageUnits = "1.3.6.1.2.1.25.2.3.1.4";
constexpr const char *StorageSize = "1.3.6.1.2.1.25.2.3.1.5";
constexpr const char *StorageUsed = "1.3.6.1.2.1.25.2.3.1.6";
constexpr const char *StorageRam = "1.3.6.1.2.1.25.2.1.2";
constexpr const char *SensorTemperature = "1.3.6.1.4.1.2021.13.16.2.1.3";
constexpr const char *SensorName = "1.3.6.1.4.1.2021.13.16.2.1.2";
// NET-SNMP-EXTEND-MIB output for the explicitly configured mos2CpuTemp script.
constexpr const char *CpuTemperature = "1.3.6.1.4.1.8072.1.3.2.3.1.1.11.109.111.115.50.67.112.117.84.101.109.112";
namespace Synology {
constexpr const char *Enterprise = "1.3.6.1.4.1.6574";
constexpr const char *Model = "1.3.6.1.4.1.6574.1.5.1.0";
constexpr const char *Temperature = "1.3.6.1.4.1.6574.1.2.0";
constexpr const char *Disks = "1.3.6.1.4.1.6574.2.1.1";
}
}
