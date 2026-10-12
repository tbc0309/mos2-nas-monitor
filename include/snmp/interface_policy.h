#pragma once
#include <stdint.h>
namespace InterfacePolicy {
// IANA ifType: Ethernet, PPP, Wi-Fi, link aggregation and bridge.
// Loopback/tunnels are excluded from automatic discovery, not explicit indexes.
inline bool selectable(uint64_t type) {
  return type==6 || type==23 || type==71 || type==161 || type==209;
}
}
