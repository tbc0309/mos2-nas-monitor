#pragma once
#include <stdint.h>
namespace Activity {
constexpr uint32_t Grace = 300000, ManagementGrace = 10000;
inline bool desktop(int page) { return page == 5 || page == 6 || page == 7; }
struct Policy {
  int lastPage = -1;
  uint32_t entered = 0, demandAt = 0;
  bool demanded = false;
  void demand(uint32_t now) { demandAt = now; demanded = true; }
  void update(uint32_t now, int page) { if (page != lastPage) { lastPage = page; entered = now; } }
  bool snmp(uint32_t now) const { return !desktop(lastPage) || uint32_t(now-entered) < Grace || (demanded && uint32_t(now-demandAt) < ManagementGrace); }
  bool weather(uint32_t now) const { return lastPage == 5 || uint32_t(now-entered) < Grace; }
};
}
