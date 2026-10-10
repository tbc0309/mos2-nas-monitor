#pragma once
#include <Arduino.h>
#include <WiFiUdp.h>
#include <vector>
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
  if (n.begin == n.end)
    return "";
  uint8_t v = b[n.begin];
  String s = String(v < 80 ? v / 40 : 2) + "." + String(v < 80 ? v % 40 : v - 80);
  uint32_t val = 0;
  for (size_t p = n.begin + 1; p < n.end; p++) {
    val = (val << 7) | (b[p] & 127);
    if (!(b[p] & 128)) {
      s += '.';
      s += String(val);
      val = 0;
    }
  }
  return s;
}
struct Value {
  String oid, text;
  uint64_t num = 0;
  uint8_t type = 0;
  bool valid = false, negative = false;
};
inline bool decode(const Bytes &b, uint32_t request, Value &value) {
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
  p = pdu.begin;
  if (!read(b, p, pdu.end, id) || id.tag != 2 || number(b, id) != request ||
      !read(b, p, pdu.end, err) || err.tag != 2 || number(b, err) || !read(b, p, pdu.end, idx) ||
      idx.tag != 2 || !read(b, p, pdu.end, list) || list.tag != 0x30)
    return false;
  p = list.begin;
  if (!read(b, p, list.end, binding) || binding.tag != 0x30)
    return false;
  p = binding.begin;
  if (!read(b, p, binding.end, o) || o.tag != 6 || !read(b, p, binding.end, v) || v.tag >= 0x80)
    return false;
  if (v.tag != 4 && v.tag != 2 && v.tag != 0x41 && v.tag != 0x42 && v.tag != 0x43 && v.tag != 0x46)
    return false;
  if (v.tag != 4 &&
      (v.end == v.begin || v.end - v.begin > 9 || (v.end - v.begin == 9 && b[v.begin] != 0)))
    return false;
  value.negative = v.tag == 2 && (b[v.begin] & 128);
  value.oid = oidText(b, o);
  value.type = v.tag;
  value.valid = true;
  if (v.tag == 4) {
    for (size_t i = v.begin; i < v.end; i++)
      value.text += char(b[i]);
  } else
    value.num = number(b, v);
  return true;
}
class Client {
  WiFiUDP udp;
  uint32_t sequence = 0, active = 0, started = 0;
  IPAddress target;
  String expected;
  bool next = false, pending = false;

public:
  uint32_t sent = 0, received = 0, timeouts = 0, invalid = 0, sendErrors = 0;
  bool bound = false;
  unsigned timeoutStreak = 0;
  void begin() {
    udp.stop();
    bound = udp.begin(0);
    pending = false;
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
    pending = false;
  }
  void query(IPAddress ip, uint16_t port, const String &community, const String &name,
             bool getNext = false) {
    target = ip;
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
    udp.beginPacket(ip, port);
    udp.write(packet.data(), packet.size());
    if (udp.endPacket())
      sent++;
    else
      sendErrors++;
    started = millis();
    pending = true;
  }
  // 0 waiting, 1 valid response, -1 timeout/error
  int poll(Value &out) {
    int len = udp.parsePacket();
    if (len) {
      if (len > 1500) {
        while (udp.available())
          udp.read();
        return 0;
      }
      Bytes b(len);
      udp.read(b.data(), len);
      if (!pending || udp.remoteIP() != target)
        return 0;
      if (decode(b, active, out) &&
          ((!next && out.oid == expected) || (next && out.oid != expected))) {
        pending = false;
        received++;
        timeoutStreak = 0;
        return 1;
      }
      invalid++;
    }
    if (pending && millis() - started > 1400) {
      pending = false;
      timeouts++;
      if (++timeoutStreak >= 3) {
        udp.stop();
        bound = udp.begin(0);
        timeoutStreak = 0;
      }
      return -1;
    }
    return 0;
  }
};
} // namespace Snmp
