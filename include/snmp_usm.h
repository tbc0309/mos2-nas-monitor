#pragma once
// Included inside namespace Snmp, after the shared BER codec.
namespace Usm {
inline Bytes text(const String &s) {
  return Bytes(s.begin(), s.end());
}
inline Bytes slice(const Bytes &b, const Node &n) {
  return Bytes(b.begin() + n.begin, b.begin() + n.end);
}
struct Message {
  uint32_t id = 0, boots = 0, time = 0;
  uint8_t flags = 0;
  Bytes engine, user;
  Node auth, pdu, privacy, scoped;
};
inline bool parse(const Bytes &b, Message &m) {
  size_t p = 0;
  Node outer, v, header, id, max, flags, model, security, seq, engine, boots, time, user, privacy,
      scoped, context, name;
  if (!read(b, p, b.size(), outer) || outer.tag != 48 || outer.end != b.size())
    return false;
  p = outer.begin;
  if (!read(b, p, outer.end, v) || v.tag != 2 || number(b, v) != 3 ||
      !read(b, p, outer.end, header) || header.tag != 48 || !read(b, p, outer.end, security) ||
      security.tag != 4 || !read(b, p, outer.end, scoped) ||
      (scoped.tag != 48 && scoped.tag != 4) || p != outer.end)
    return false;
  p = header.begin;
  if (!read(b, p, header.end, id) || id.tag != 2 || id.end - id.begin > 4 ||
      !read(b, p, header.end, max) || max.tag != 2 || !read(b, p, header.end, flags) ||
      flags.tag != 4 || flags.end - flags.begin != 1 || !read(b, p, header.end, model) ||
      model.tag != 2 || number(b, model) != 3 || p != header.end)
    return false;
  m.id = number(b, id);
  m.flags = b[flags.begin];
  if ((m.flags & ~7) || ((m.flags & 2) && !(m.flags & 1)))
    return false; // Privacy requires authentication.
  p = security.begin;
  if (!read(b, p, security.end, seq) || seq.tag != 48 || p != security.end)
    return false;
  p = seq.begin;
  if (!read(b, p, seq.end, engine) || engine.tag != 4 || engine.end - engine.begin > 32 ||
      !read(b, p, seq.end, boots) || boots.tag != 2 || boots.end - boots.begin > 4 ||
      !read(b, p, seq.end, time) || time.tag != 2 || time.end - time.begin > 4 ||
      !read(b, p, seq.end, user) || user.tag != 4 || user.end - user.begin > 32 ||
      !read(b, p, seq.end, m.auth) || m.auth.tag != 4 || !read(b, p, seq.end, privacy) ||
      privacy.tag != 4 || privacy.end - privacy.begin != ((m.flags & 2) ? 8u : 0u) || p != seq.end)
    return false;
  m.engine = slice(b, engine);
  m.user = slice(b, user);
  m.boots = number(b, boots);
  m.time = number(b, time);
  if (m.boots >= 0x7fffffff || m.time >= 0x7fffffff)
    return false;
  m.privacy = privacy;
  m.scoped = scoped;
  if (m.flags & 2)
    return scoped.tag == 4;
  if (scoped.tag != 48)
    return false;
  p = scoped.begin;
  if (!read(b, p, scoped.end, context) || context.tag != 4 || !read(b, p, scoped.end, name) ||
      name.tag != 4 || name.begin != name.end || !read(b, p, scoped.end, m.pdu) || p != scoped.end)
    return false;
  return slice(b, context) == m.engine;
}

struct Session {
  String user, password, privacyPassword;
  bool privacyEnabled = false, privacyReady = false, sha = false, aesPrivacy = false;
  uint8_t privacyKu[20] = {}, privacyKey[20] = {};
  uint64_t salt = 0;
  bool enabled = false, keyReady = false, discovering = false;
  Bytes engine;
  uint8_t ku[20] = {}, key[20] = {};
  uint32_t boots = 0, time = 0, synced = 0, stretched = 0, messageId = 0, lastResponse = 0;
  unsigned passwordAt = 0;
  SnmpCrypto::Hash stretch;
  String error;
  void configure(bool v3, const String &u, const String &p, const String &privacy = "none",
                 const String &privPassword = "", uint64_t saltSeed = 0,
                 const String &auth = "MD5") {
    enabled = v3;
    sha = auth == "SHA";
    aesPrivacy = privacy == "AES";
    privacyEnabled = v3 && (privacy == "DES" || aesPrivacy);
    privacyPassword = privPassword;
    privacyReady = !privacyEnabled;
    salt = saltSeed;
    memset(privacyKu, 0, sizeof privacyKu);
    memset(privacyKey, 0, sizeof privacyKey);
    user = u;
    password = p;
    keyReady = false;
    engine.clear();
    discovering = false;
    boots = time = synced = stretched = passwordAt = lastResponse = 0;
    stretch = SnmpCrypto::Hash(sha);
    error = "";
    memset(ku, 0, sizeof ku);
    memset(key, 0, sizeof key);
  }
  // Stretch only once per configuration, in small cooperative loop slices.
  void step() {
    if (!enabled || (keyReady && privacyReady) || password.length() < 8)
      return;
    const String &activePassword = keyReady ? privacyPassword : password;
    if (activePassword.length() < 8)
      return;
    uint8_t part[64];
    for (unsigned chunk = 0; chunk < 64 && stretched < 1048576; chunk++) {
      for (auto &v : part) {
        v = activePassword[passwordAt++];
        if (passwordAt == activePassword.length())
          passwordAt = 0;
      }
      stretch.add(part, 64);
      stretched += 64;
    }
    if (stretched == 1048576) {
      if (!keyReady) {
        stretch.finish(ku);
        keyReady = true;
        if (privacyEnabled && privacyPassword == password) {
          memcpy(privacyKu, ku, sha ? 20 : 16);
          privacyReady = true;
        }
      } else {
        stretch.finish(privacyKu);
        privacyReady = true;
      }
      stretched = passwordAt = 0;
      stretch = SnmpCrypto::Hash(sha);
    }
  }
  uint32_t clock() const {
    return time + uint32_t(millis() - synced) / 1000;
  }
  void sync(const Message &m) {
    if (engine != m.engine) {
      engine = m.engine;
      SnmpCrypto::Hash localized(sha);
      localized.add(ku, sha ? 20 : 16);
      localized.add(engine.data(), engine.size());
      localized.add(ku, sha ? 20 : 16);
      localized.finish(key);
      SnmpCrypto::Hash privateLocalized(sha);
      privateLocalized.add(privacyKu, sha ? 20 : 16);
      privateLocalized.add(engine.data(), engine.size());
      privateLocalized.add(privacyKu, sha ? 20 : 16);
      privateLocalized.finish(privacyKey);
    }
    boots = m.boots;
    time = m.time;
    synced = millis();
    lastResponse = millis();
    error = "";
  }
  void makeIv(uint32_t engineBoots, uint32_t engineTime, const uint8_t *parameters,
              uint8_t *iv) const {
    if (aesPrivacy) {
      for (unsigned i = 0; i < 4; i++) {
        iv[i] = engineBoots >> (24 - 8 * i);
        iv[i + 4] = engineTime >> (24 - 8 * i);
      }
      memcpy(iv + 8, parameters, 8);
    } else
      for (unsigned i = 0; i < 8; i++)
        iv[i] = privacyKey[i + 8] ^ parameters[i];
  }
  Bytes packet(uint32_t id, uint32_t request, const String &oidName, bool next, bool discovery) {
    const uint32_t messageTime = clock();
    Bytes binding = oid(oidName);
    append(binding, tlv(5, {}));
    Bytes data = integer(request);
    append(data, integer(0));
    append(data, integer(0));
    append(data, tlv(48, tlv(48, binding)));
    Bytes scoped = tlv(4, discovery ? Bytes{} : engine);
    append(scoped, tlv(4, {}));
    append(scoped, tlv(next ? 0xa1 : 0xa0, data));
    scoped = tlv(48, scoped);
    Bytes privateParameters;
    if (!discovery && privacyEnabled) {
      privateParameters.resize(8);
      if (aesPrivacy) {
        for (unsigned i = 0; i < 8; i++)
          privateParameters[i] = salt >> (56 - 8 * i);
      } else {
        for (unsigned i = 0; i < 4; i++) {
          privateParameters[i] = boots >> (24 - 8 * i);
          privateParameters[i + 4] = salt >> (24 - 8 * i);
        }
        scoped.resize((scoped.size() + 7) & ~size_t(7), 0);
      }
      ++salt;
      uint8_t iv[16];
      makeIv(boots, messageTime, privateParameters.data(), iv);
      if (!(aesPrivacy ? SnmpCrypto::aes(true, privacyKey, iv, scoped.data(), scoped.size())
                       : SnmpCrypto::des(true, privacyKey, iv, scoped.data(), scoped.size())))
        return {};
      scoped = tlv(4, scoped);
    }
    Bytes security = tlv(4, discovery ? Bytes{} : engine);
    append(security, integer(discovery ? 0 : boots));
    append(security, integer(discovery ? 0 : messageTime));
    append(security, tlv(4, discovery ? Bytes{} : text(user)));
    append(security, tlv(4, discovery ? Bytes{} : Bytes(12, 0)));
    append(security, tlv(4, privateParameters));
    Bytes header = integer(id);
    append(header, integer(1500));
    append(header, tlv(4, Bytes{uint8_t(discovery ? 4 : (privacyEnabled ? 7 : 5))}));
    append(header, integer(3));
    Bytes out = integer(3);
    append(out, tlv(48, header));
    append(out, tlv(4, tlv(48, security)));
    append(out, scoped);
    out = tlv(48, out);
    if (!discovery) {
      Message m;
      if (!parse(out, m))
        return {};
      uint8_t digest[20];
      SnmpCrypto::authenticate(sha, key, out.data(), out.size(), digest);
      memcpy(out.data() + m.auth.begin, digest, 12);
    }
    return out;
  }
  bool authenticated(Bytes &b, const Message &m) const {
    if (!(m.flags & 1) || m.auth.end - m.auth.begin != 12 || m.engine != engine ||
        m.user != text(user))
      return false;
    uint8_t expected[12], digest[20];
    memcpy(expected, b.data() + m.auth.begin, 12);
    memset(b.data() + m.auth.begin, 0, 12);
    SnmpCrypto::authenticate(sha, key, b.data(), b.size(), digest);
    memcpy(b.data() + m.auth.begin, expected, 12);
    unsigned mismatch = 0;
    for (unsigned i = 0; i < 12; i++)
      mismatch |= expected[i] ^ digest[i];
    return mismatch == 0;
  }
  // Reuse the strict v2 value decoder after authenticating the complete v3 envelope.
  bool value(const Bytes &b, const Message &m, uint32_t request, Value &out) const {
    Bytes body = integer(1);
    append(body, tlv(4, {}));
    append(body, tlv(0xa2, slice(b, m.pdu)));
    return decode(tlv(48, body), request, out);
  }
  int accept(Bytes &b, uint32_t id, uint32_t request, bool discovery, Value &out) {
    Message m;
    if (!parse(b, m) || m.id != id)
      return 0;
    if (discovery) {
      // Engine discovery is the only permitted unauthenticated report.
      if (m.flags & 2 || m.pdu.tag != 0xa8 || m.flags & 1 || m.auth.begin != m.auth.end ||
          m.engine.size() < 5)
        return 0;
      Value report;
      if (!value(b, m, request, report) || report.oid != "1.3.6.1.6.3.15.1.1.4.0")
        return 0;
      sync(m);
      return -1;
    }
    if (!authenticated(b, m)) {
      error = "authentication";
      return 0;
    }
    Bytes plain;
    const Bytes *payload = &b;
    if (m.flags & 2) {
      if (!privacyEnabled)
        return 0;
      plain = slice(b, m.scoped);
      uint8_t iv[16];
      makeIv(m.boots, m.time, b.data() + m.privacy.begin, iv);
      if (!(aesPrivacy ? SnmpCrypto::aes(false, privacyKey, iv, plain.data(), plain.size())
                       : SnmpCrypto::des(false, privacyKey, iv, plain.data(), plain.size()))) {
        error = "privacy";
        return 0;
      }
      size_t pos = 0;
      Node scope, context, name;
      if (!read(plain, pos, plain.size(), scope) || scope.tag != 48 ||
          plain.size() - scope.end > (aesPrivacy ? 0u : 7u)) {
        error = "privacy";
        return 0;
      }
      pos = scope.begin;
      if (!read(plain, pos, scope.end, context) || context.tag != 4 ||
          slice(plain, context) != engine || !read(plain, pos, scope.end, name) || name.tag != 4 ||
          name.begin != name.end || !read(plain, pos, scope.end, m.pdu) || pos != scope.end) {
        error = "privacy";
        return 0;
      }
      payload = &plain;
    } else if (privacyEnabled && m.pdu.tag != 0xa8) {
      error = "privacy_required";
      return 0;
    }
    if (m.pdu.tag == 0xa8) {
      Value report;
      if (!value(*payload, m, request, report))
        return 0;
      if (report.oid == "1.3.6.1.6.3.15.1.1.2.0" && m.boots >= boots) {
        sync(m);
        error = "resync";
        return -1;
      }
      error = "report";
      return -1;
    }
    uint32_t expected = clock();
    if (m.pdu.tag != 0xa2 || m.boots != boots || m.time + 150 < expected || m.time > expected + 150)
      return 0;
    if (!value(*payload, m, request, out))
      return 0;
    lastResponse = millis();
    error = "";
    // Old packets cannot move the authoritative clock backwards.
    if (m.time >= expected)
      sync(m);
    return 1;
  }
};
inline Session &session() {
  static Session s;
  return s;
}
} // namespace Usm
