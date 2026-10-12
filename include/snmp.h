#pragma once
#include "sampling_policy.h"
#include "storage_order.h"
#include <Arduino.h>
#include <WiFiUdp.h>
#include <vector>
#include "snmp_hash.h"
#include "snmp_des.h"
namespace Snmp {
using Bytes = std::vector<uint8_t>;
inline void append(Bytes &a, const Bytes &b) {
  a.insert(a.end(), b.begin(), b.end());
}
inline Bytes tlv(uint8_t tag, const Bytes &b) {
  Bytes r{tag};
  if (b.size() < 128)
    r.push_back(b.size());
  else {
    r.push_back(0x82);
    r.push_back(b.size() >> 8);
    r.push_back(b.size());
  }
  append(r, b);
  return r;
}
inline Bytes integer(uint32_t v) {
  Bytes b;
  do {
    b.insert(b.begin(), v & 255);
    v >>= 8;
  } while (v);
  if (b[0] & 128)
    b.insert(b.begin(), 0);
  return tlv(2, b);
}
inline Bytes oid(const String &s) {
  Bytes b;
  unsigned start = 0, index = 0;
  uint32_t first = 0;
  while (start < s.length()) {
    int end = s.indexOf('.', start);
    if (end < 0)
      end = s.length();
    uint32_t n = s.substring(start, end).toInt();
    start = end + 1;
    if (index++ == 0) {
      first = n;
      continue;
    }
    if (index == 2) {
      b.push_back(first * 40 + n);
      continue;
    }
    Bytes part{uint8_t(n & 127)};
    while (n >>= 7)
      part.insert(part.begin(), uint8_t((n & 127) | 128));
    append(b, part);
  }
  return tlv(6, b);
}
struct Node {
  uint8_t tag;
  size_t begin, end;
};
inline bool read(const Bytes &b, size_t &p, size_t limit, Node &n) {
  if (limit > b.size() || p > limit || limit - p < 2)
    return false;
  n.tag = b[p++];
  size_t len = b[p++];
  if (len & 128) {
    unsigned count = len & 127;
    if (!count || count > 2 || p + count > limit)
      return false;
    len = 0;
    while (count--)
      len = (len << 8) | b[p++];
  }
  if (len > limit - p)
    return false;
  n.begin = p;
  n.end = p + len;
  p = n.end;
  return true;
}
inline uint64_t number(const Bytes &b, const Node &n) {
  uint64_t v = 0;
  for (size_t p = n.begin; p < n.end; p++)
    v = (v << 8) | b[p];
  return v;
}
inline String oidText(const Bytes &b, const Node &n) {
  if (n.begin >= n.end || n.end > b.size() || n.end - n.begin > 128) return "";
  String text;
  uint64_t value = 0;
  unsigned bytes = 0, arcs = 0;
  for (size_t p = n.begin; p < n.end; ++p) {
    value = (value << 7) | (b[p] & 127);
    if (++bytes > 5 || value > (arcs ? 0xffffffffULL : 0xffffffffULL + 80)) return "";
    if (!(b[p] & 128)) {
      if (!arcs) {
        unsigned first = value < 40 ? 0 : value < 80 ? 1 : 2;
        text = String(first) + "." + String(uint32_t(value - first * 40));
      } else text += "." + String(uint32_t(value));
      if (++arcs > 64) return "";
      value = 0; bytes = 0;
    }
  }
  return bytes ? String("") : text; // A trailing continuation byte is not a complete arc.
}
struct Value {
  String oid, text;
  uint64_t num = 0;
  uint8_t type = 0;
  bool valid = false, negative = false;
};
inline bool decode(const Bytes &b, uint32_t request, Value &value, bool exceptions = false, const String *expectedCommunity = nullptr) {
  value = {};
  size_t p = 0;
  Node outer, ver, community, pdu, id, err, idx, list, binding, o, v;
  if (!read(b, p, b.size(), outer) || outer.tag != 0x30 || outer.end != b.size())
    return false;
  p = outer.begin;
  if (!read(b, p, outer.end, ver) || ver.tag != 2 || number(b, ver) != 1 ||
      !read(b, p, outer.end, community) || community.tag != 4 || !read(b, p, outer.end, pdu) ||
      pdu.tag != 0xa2)
    return false;
  if (expectedCommunity && (community.end - community.begin != expectedCommunity->length() ||
      memcmp(b.data() + community.begin, expectedCommunity->c_str(), expectedCommunity->length()))) return false;
  p = pdu.begin;
  if (!read(b, p, pdu.end, id) || id.tag != 2 || number(b, id) != request ||
      !read(b, p, pdu.end, err) || err.tag != 2 || number(b, err) || !read(b, p, pdu.end, idx) ||
      idx.tag != 2 || !read(b, p, pdu.end, list) || list.tag != 0x30 || p != pdu.end)
    return false;
  p = list.begin;
  if (!read(b, p, list.end, binding) || binding.tag != 0x30 || p != list.end)
    return false;
  p = binding.begin;
  if (!read(b, p, binding.end, o) || o.tag != 6 || !read(b, p, binding.end, v) || p != binding.end)
    return false;
  if (exceptions && v.tag >= 0x80 && v.tag <= 0x82 && v.begin == v.end) {
    value.oid = oidText(b, o);
    value.type = v.tag;
    value.negative = true;
    return true;
  }
  if (v.tag != 6 && v.tag != 4 && v.tag != 2 && v.tag != 0x41 && v.tag != 0x42 && v.tag != 0x43 && v.tag != 0x46)
    return false;
  if (v.tag != 4 && v.tag != 6 &&
      (v.end == v.begin || v.end - v.begin > 9 || (v.end - v.begin == 9 && b[v.begin] != 0)))
    return false;
  value.negative = v.tag == 2 && (b[v.begin] & 128);
  value.oid = oidText(b, o);
  value.type = v.tag;
  value.valid = true;
  if (v.tag == 4) {
    for (size_t i = v.begin; i < v.end; i++)
      value.text += char(b[i]);
  } else if (v.tag == 6) {
    value.text = oidText(b, v);
    if (!value.text.length()) return false;
  } else
    value.num = number(b, v);
  return true;
}
#include "snmp_usm.h"
class Client {
  WiFiUDP udp;
  uint32_t sequence = 0, active = 0, started = 0;
  IPAddress target;
  String expected, responseCommunity;
  uint16_t targetPort = 161;
  bool next = false, pending = false, discovery = false, v3 = false;
  bool localFailure = false;
  uint32_t messageId = 0;
  static unsigned &inFlight() {
    static unsigned n = 0;
    return n;
  }
  static uint32_t &lastSend() {
    static uint32_t n = 0;
    return n;
  }
  static bool &hasSent() {
    static bool n = false;
    return n;
  }

  static std::vector<Client *> &waiters() {
    static std::vector<Client *> q; return q;
  }
  static uint32_t &cooldownUntil() { static uint32_t n = 0; return n; }
  static unsigned &globalFailures() { static unsigned n = 0; return n; }

public:
  static void resetTransportBudget() {
    // Called only after cancelling all collectors when the target changes.
    // Keep send spacing; a dead target must not put the next target in cooldown.
    globalFailures()=0;cooldownUntil()=0;
  }
  uint32_t sent = 0, received = 0, timeouts = 0, invalid = 0, sendErrors = 0;
  bool bound = false;
  unsigned timeoutStreak = 0;
  void begin() {
    cancel();
    udp.stop();
    bound = false; // Bind only while an actual request owns the global slot.
    pending = false;
    localFailure = false;
  }
  String diagnostics() const {
    return String("\"sent\":") + String(sent) + ",\"received\":" + String(received) +
           ",\"timeouts\":" + String(timeouts) + ",\"invalid\":" + String(invalid) +
           ",\"sendErrors\":" + String(sendErrors) +
           ",\"pending\":" + (pending ? "true" : "false") +
           ",\"bound\":" + (bound ? "true" : "false");
  }
  bool busy() const {
    return pending;
  }
  void cancel() {
    localFailure=false;
    auto &q = waiters();
    for (size_t i = 0; i < q.size(); ++i)
      if (q[i] == this) { q.erase(q.begin() + i); break; }
    if (pending)
      --inFlight();
    if (pending && discovery)
      Usm::session().discovering = false;
    pending = false;
    udp.stop(); bound = false;
    expected = responseCommunity = "";
  }
  bool query(IPAddress ip, uint16_t port, const String &community, const String &name,
             bool getNext = false) {
    // One global request at a time protects the NAS. Repeated timeouts pause
    // every collector; after cooldown only one probe can resume traffic.
    uint32_t now = millis();
    if (pending) return false;
    auto &q = waiters();
    bool queued = false;
    for (auto *c : q) queued |= c == this;
    if (!queued) q.push_back(this);
    if (q.front() != this) return false;
    if ((cooldownUntil() && int32_t(now - cooldownUntil()) < 0) || pending || inFlight() >= Sampling::MaxPending || (hasSent() && now - lastSend() < Sampling::SendSpacing))
      return false;
    auto &usm = Usm::session();
    v3 = usm.enabled;
    discovery = v3 && usm.engine.empty();
    if (v3 && ((!usm.keyReady || !usm.privacyReady) || (discovery && usm.discovering)))
      return false;
    target = ip; targetPort = port; responseCommunity = community;
    next = getNext;
    expected = name;
    active = ++sequence;
    Bytes binding = oid(name);
    append(binding, tlv(5, {}));
    Bytes list = tlv(0x30, tlv(0x30, binding));
    Bytes data = integer(active);
    append(data, integer(0));
    append(data, integer(0));
    append(data, list);
    Bytes packet = integer(1);
    Bytes c(community.begin(), community.end());
    append(packet, tlv(4, c));
    append(packet, tlv(next ? 0xa1 : 0xa0, data));
    packet = tlv(0x30, packet);
    if (v3) {
      messageId = ++usm.messageId;
      packet = usm.packet(messageId, active, name, next, discovery);
      if (packet.empty()) { ++sendErrors;cancel();localFailure=true;return false; }
      if (discovery)
        usm.discovering = true;
    }
    if (!bound) bound = udp.begin(0);
    if (!bound) { ++sendErrors;cancel();localFailure=true;return false; }
    udp.beginPacket(ip, port);
    udp.write(packet.data(), packet.size());
    if (udp.endPacket())
      sent++;
    else { ++sendErrors;cancel();localFailure=true;return false; }
    started = millis();
    pending = true;
    ++inFlight();
    q.erase(q.begin());
    lastSend() = now;
    hasSent() = true;
    return true;
  }
  // 0 waiting, 1 valid response, -1 timeout/error
  int poll(Value &out) {
    if(localFailure) {localFailure=false;return -1;}
    int len = udp.parsePacket();
    if (len) {
      if (len > 1500) {
        while (udp.available())
          udp.read();
        return 0;
      }
      Bytes b(len);
      udp.read(b.data(), len);
      if (!pending || udp.remoteIP() != target || udp.remotePort() != targetPort)
        return 0;
      int result = v3 ? Usm::session().accept(b, messageId, active, discovery, out)
                        : (decode(b, active, out, true, &responseCommunity) ? 1 : 0);
      if (result == 1 && out.type >= 0x80 && out.type <= 0x82 && out.oid == expected) {
        cancel();
        received++;
        globalFailures()=0;cooldownUntil()=0;timeoutStreak=0;
        // A supported agent explicitly rejected this OID; do not wait/retry or
        // count it as a network timeout that trips the global cooldown.
        return 1;
      }
      if (result < 0) {
        // A validated engine report is setup, not a failed data sample.
        // Keep the collector cursor so its actual GET can follow immediately.
        bool engineReady = discovery && !Usm::session().engine.empty();
        cancel();
        return engineReady ? 0 : -1;
      }
      if (result == 1 && ((!next && out.oid == expected) || (next && StorageOrder::less(expected.c_str(), out.oid.c_str())))) {
        cancel();
        received++;
        globalFailures() = 0;
        cooldownUntil() = 0;
        timeoutStreak = 0;
        return 1;
      }
      invalid++;
    }
    // Synology extension OIDs can reply after the old 1.4-second deadline.
    // Waiting here is nonblocking: display, touch and web handling keep running.
    if (pending && millis() - started > Sampling::RequestTimeout) {
      cancel();
      timeouts++;
      if (++globalFailures() >= 3) {
        cooldownUntil() = millis() + 60000u;
      }
      if (++timeoutStreak >= 3) {
        if (v3 && !Usm::session().discovering && millis() - Usm::session().lastResponse > 30000)
          Usm::session().engine.clear();
        udp.stop();
        bound = false;
        timeoutStreak = 0;
      }
      return -1;
    }
    return 0;
  }
};
} // namespace Snmp
