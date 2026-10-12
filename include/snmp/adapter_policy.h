#pragma once
#include "device_profile.h"
namespace SnmpAdapter {
// A system label is evidence, not permission to invent vendor measurements.
// Add vendor capabilities here only after validating the vendor's MIB and device.
struct Policy {
  bool vendorDiskTable;
  bool vendorTemperature;
  bool freeAndCacheMemory;
  bool includeRootVolume;
};
inline Policy policy(DeviceProfile::Match match) {
  // Descriptions can contain administrator-selected hostnames. Require stronger
  // evidence before enabling proprietary queries or vendor-specific arithmetic.
  if (match.system==DeviceProfile::System::Synology &&
      (match.evidence==DeviceProfile::Evidence::Enterprise ||
       match.evidence==DeviceProfile::Evidence::VendorResponse)) return {true,true,true,false};
  return {false,false,false,true};
}
}
