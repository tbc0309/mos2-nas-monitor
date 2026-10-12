#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Independent of HTTP and Arduino: the same limits can be exercised on a host.
namespace AccessPolicy {
constexpr uint32_t SessionTtl = 1800000;
// A successful new login replaces the previous management session.
class Sessions {
  struct Entry { char token[49] = {}; uint32_t seen = 0; } entries[1];
public:
  bool valid(const char *token, uint32_t now) {
    if (!token || strlen(token) != 48) return false;
    for (auto &e : entries) {
      if (!strcmp(token, e.token) && uint32_t(now - e.seen) <= SessionTtl) {
        e.seen = now; return true;
      }
    }
    return false;
  }
  void issue(const char *token, uint32_t now) {
    if (!token || strlen(token) != 48) return;
    clear();
    memcpy(entries[0].token, token, 49); entries[0].seen = now;
  }
  void revoke(const char *token) {
    for (auto &e : entries) if (token && !strcmp(token, e.token)) e.token[0] = 0;
  }
  void clear() { for (auto &e : entries) e.token[0] = 0; }
};
struct LoginGuard {
  unsigned failures = 0;
  uint32_t blockedAt = 0, wait = 0;
  bool blocked(uint32_t now) const { return wait && uint32_t(now - blockedAt) < wait; }
  void failed(uint32_t now) {
    if (failures < 10) ++failures;
    if (failures >= 5) {
      wait = 30000u << (failures - 5);
      if (wait > 300000u) wait = 300000u;
      blockedAt = now;
    }
  }
  void clear() { failures = 0; blockedAt = wait = 0; }
};
inline bool decimal(const char *s, unsigned limit, unsigned &value) {
  if (!s || !*s) return false;
  value = 0;
  for (; *s; ++s) {
    if (*s < '0' || *s > '9' || unsigned(*s - '0') > limit || value > (limit - unsigned(*s - '0')) / 10) return false;
    value = value * 10 + unsigned(*s - '0');
  }
  return true;
}
}
