#pragma once
#include <cstring>
#include <cstdint>
#include <initializer_list>
namespace TemperaturePolicy {
// LM-SENSORS-MIB indexes are agent-specific. Rank labels, never row numbers,
// IP addresses or hostnames. Unknown labels remain generic device readings.
inline unsigned rank(const char *label) {
  char text[97];unsigned n=0;
  for(;label[n] && n<96;++n) {
    const unsigned char c=label[n];
    text[n]=c>='A'&&c<='Z'?char(c+'a'-'A'):char(c);
  }
  text[n]=0;
  for(const char *word:{"disk","drive","nvme","ssd","hdd"})
    if(std::strstr(text,word)) return 0;
  for(const char *word:{"package","tdie","tctl","cpu","soc_dts"})
    if(std::strstr(text,word)) return 100;
  if(std::strstr(text,"core")) return 70;
  if(std::strstr(text,"system") || std::strstr(text,"board") || std::strstr(text,"ambient")) return 30;
  return 10;
}
constexpr unsigned MaxRows=16;
constexpr uint64_t MaxMillidegrees=150000;
}
