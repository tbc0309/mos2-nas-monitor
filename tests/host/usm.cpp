#include "snmp.h"
#include <cassert>
#include <cstdio>
using namespace Snmp;
String hex(const uint8_t *b, unsigned n) {
  char v[3];
  String s;
  for (unsigned i = 0; i < n; i++) {
    sprintf(v, "%02x", b[i]);
    s += v;
  }
  return s;
}
Bytes reply(Usm::Session &s, uint32_t id, uint32_t request, const String &name, uint8_t tag = 0xa2,
            bool discovery = false) {
  Usm::Message m;
  Bytes body = integer(request);
  append(body, integer(0));
  append(body, integer(0));
  Bytes pair = oid(name);
  append(pair, integer(42));
  append(body, tlv(48, tlv(48, pair)));
  // Rebuild the envelope to account for the changed value length.
  Bytes scoped = tlv(4, s.engine);
  append(scoped, tlv(4, {}));
  append(scoped, tlv(tag, body));
  scoped = tlv(48, scoped);
  Bytes salt;
  if (!discovery && s.privacyEnabled) {
    salt = {0, 0, 0, 1, 0, 0, 0, 7};
    uint8_t iv[16];
    s.makeIv(s.boots, s.clock(), salt.data(), iv);
    if (!s.aesPrivacy)
      scoped.resize((scoped.size() + 7) & ~size_t(7), 0);
    assert(s.aesPrivacy ? SnmpCrypto::aes(true, s.privacyKey, iv, scoped.data(), scoped.size())
                        : SnmpCrypto::des(true, s.privacyKey, iv, scoped.data(), scoped.size()));
    scoped = tlv(4, scoped);
  }
  Bytes security = tlv(4, s.engine);
  append(security, integer(s.boots));
  append(security, integer(s.clock()));
  append(security, tlv(4, discovery ? Bytes{} : Usm::text(s.user)));
  append(security, tlv(4, discovery ? Bytes{} : Bytes(12, 0)));
  append(security, tlv(4, salt));
  Bytes header = integer(id);
  append(header, integer(1500));
  append(header, tlv(4, Bytes{uint8_t(discovery ? 0 : (s.privacyEnabled ? 3 : 1))}));
  append(header, integer(3));
  Bytes out = integer(3);
  append(out, tlv(48, header));
  append(out, tlv(4, tlv(48, security)));
  append(out, scoped);
  out = tlv(48, out);
  assert(Usm::parse(out, m));
  uint8_t digest[20];
  SnmpCrypto::authenticate(s.sha, s.key, out.data(), out.size(), digest);
  if (!discovery)
    memcpy(out.data() + m.auth.begin, digest, 12);
  return out;
}
int main() {
  uint8_t digest[20];
  SnmpCrypto::Md5 md5;
  md5.add((const uint8_t *)"abc", 3);
  md5.finish(digest);
  assert(hex(digest, 16) == "900150983cd24fb0d6963f7d28e17f72");
  Usm::Session s;
  s.configure(true, "test", "maplesyrup");
  for (unsigned i = 0; i < 255; i++) {
    s.step();
    assert(!s.keyReady);
  }
  s.step();
  assert(s.keyReady);
  assert(hex(s.ku, 16) == "9faf3283884e92834ebc9847d8edd963");
  Usm::Message m;
  m.engine = Bytes(12, 0);
  m.engine[11] = 2;
  m.boots = 1;
  m.time = 12;
  s.sync(m);
  assert(hex(s.key, 16) == "526f5eed9fcce26f8964c2930787d82b");
  auto packet = s.packet(7, 3, "1.3.6.1.2.1.1.3.0", false, false);
  assert(Usm::parse(packet, m));
  assert(s.authenticated(packet, m));
  for (size_t i = 0; i < packet.size(); i++) {
    auto shortPacket = Bytes(packet.begin(), packet.begin() + i);
    assert(!Usm::parse(shortPacket, m));
  }
  auto discoveryReport = reply(s, 7, 3, "1.3.6.1.6.3.15.1.1.4.0", 0xa8, true);
  auto fresh = s;
  fresh.engine.clear();
  Value discovered;
  assert(fresh.accept(discoveryReport, 7, 3, true, discovered) == -1 && fresh.engine == s.engine);
  assert(fresh.accept(discoveryReport, 7, 3, false, discovered) == 0);
  // The transport must not turn successful discovery into collector backoff.
  Usm::session() = fresh;
  Usm::session().engine.clear();
  Snmp::Client startupClient;
  startupClient.begin();
  testMillis += Sampling::SendSpacing;
  assert(startupClient.query({}, 161, "", "1.3.6.1.2.1.1.3.0"));
  testUdpPacket = reply(s, Usm::session().messageId, 1,
                        "1.3.6.1.6.3.15.1.1.4.0", 0xa8, true);
  assert(startupClient.poll(discovered) == 0 && !startupClient.busy());
  assert(!Usm::session().engine.empty() && startupClient.timeouts == 0);
  testMillis += Sampling::SendSpacing;
  assert(startupClient.query({}, 161, "", "1.3.6.1.2.1.1.3.0"));
  startupClient.cancel();

  auto b = reply(s, 7, 3, "1.3.6.1.2.1.1.3.0");
  Value out;
  assert(s.accept(b, 7, 3, false, out) == 1 && out.num == 42);
  assert(s.accept(b, 8, 3, false, out) == 0);
  assert(s.accept(b, 7, 4, false, out) == 0);
  auto tampered = b;
  tampered.back() ^= 1;
  assert(s.accept(tampered, 7, 3, false, out) == 0);
  auto stale = reply(s, 7, 3, "1.3.6.1.2.1.1.3.0");
  testMillis = 200000;
  assert(s.accept(stale, 7, 3, false, out) == 0);
  testMillis = 0;
  auto report = reply(s, 7, 3, "1.3.6.1.6.3.15.1.1.2.0", 0xa8);
  assert(s.accept(report, 7, 3, false, out) == -1);
  auto wrongUser = s;
  wrongUser.user = "other";
  b = reply(wrongUser, 7, 3, "1.3.6.1.2.1.1.3.0");
  assert(s.accept(b, 7, 3, false, out) == 0);
  uint8_t desKey[8] = {0x13, 0x34, 0x57, 0x79, 0x9b, 0xbc, 0xdf, 0xf1};
  uint8_t block[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}, iv[8] = {};
  assert(SnmpCrypto::des(true, desKey, iv, block, 8));
  assert(hex(block, 8) == "85e813540f0ab405");
  memset(iv, 0, 8);
  assert(SnmpCrypto::des(false, desKey, iv, block, 8));
  assert(hex(block, 8) == "0123456789abcdef");
  auto privateSession = s;
  privateSession.configure(true, "test", "maplesyrup", "DES", "differentPassword", 123);
  for (unsigned i = 0; i < 512; i++)
    privateSession.step();
  assert(privateSession.keyReady && privateSession.privacyReady);
  privateSession.sync(m); // Use the same discovered engine for both localized keys.
  auto encrypted = reply(privateSession, 7, 3, "1.3.6.1.2.1.1.3.0");
  assert(privateSession.accept(encrypted, 7, 3, false, out) == 1 && out.num == 42);
  encrypted.back() ^= 1;
  assert(privateSession.accept(encrypted, 7, 3, false, out) == 0 &&
         privateSession.error == "authentication");
  auto downgrade = reply(s, 7, 3, "1.3.6.1.2.1.1.3.0");
  assert(privateSession.accept(downgrade, 7, 3, false, out) == 0);
  uint8_t aesKey[16] = {0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
                        0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c};
  uint8_t aesIv[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  uint8_t aesBlock[16] = {0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
                          0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a};
  assert(SnmpCrypto::aes(true, aesKey, aesIv, aesBlock, 16));
  assert(hex(aesBlock, 16) == "3b3fd92eb72dad20333449f8e83cfb4a");
  for (const char *auth : {"MD5", "SHA"})
    for (const char *privacy : {"DES", "AES"}) {
      Usm::Session combo;
      combo.configure(true, "test", "maplesyrup", privacy, "differentPassword",
                      0x123456789abcdef0ULL, auth);
      for (unsigned i = 0; i < 512; i++)
        combo.step();
      assert(combo.keyReady && combo.privacyReady);
      Usm::Message engine;
      engine.engine = Bytes(12, 0);
      engine.engine[11] = 2;
      engine.boots = 1;
      engine.time = 12;
      combo.sync(engine);
      if (combo.sha) {
        assert(hex(combo.ku, 20) == "9fb5cc0381497b3793528939ff788d5d79145211");
        assert(hex(combo.key, 20) == "6695febc9288e36282235fc7151f128497b38f3f");
      }
      auto requestPacket = combo.packet(7, 3, "1.3.6.1.2.1.1.3.0", false, false);
      Usm::Message requestMessage;
      assert(Usm::parse(requestPacket, requestMessage));
      assert(combo.authenticated(requestPacket, requestMessage));
      auto response = reply(combo, 7, 3, "1.3.6.1.2.1.1.3.0");
      assert(combo.accept(response, 7, 3, false, out) == 1 && out.num == 42);
      response.back() ^= 1;
      assert(combo.accept(response, 7, 3, false, out) == 0 && combo.error == "authentication");
    }
  puts("SNMP v3 SHA/AES vectors and four auth/privacy combinations passed.");
  puts("SNMP v3 MD5 vectors, packet authentication, truncation and time window passed.");
}
