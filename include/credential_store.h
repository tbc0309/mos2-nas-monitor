#pragma once
#include <bearssl/bearssl.h>
// AES-256-CTR + HMAC-SHA256, encrypt-then-MAC. Unique 96-bit nonce per write.
// Keys are random per installation, not derived from MAC addresses or passwords.
namespace Credentials {
constexpr unsigned Rounds = 4000;
inline uint32_t lastDeriveMicros = 0;
inline bool equal(const uint8_t *a, const uint8_t *b, size_t n) {
  uint8_t difference = 0; while (n--) difference |= *a++ ^ *b++; return difference == 0;
}
inline void randomBytes(uint8_t *out, unsigned n) {
  while (n) { uint32_t word = ESP.random(); unsigned take = min(n, 4u); memcpy(out, &word, take); out += take; n -= take; }
}
inline String hex(const uint8_t *p, unsigned n) {
  String s; s.reserve(n * 2); const char *digits = "0123456789abcdef";
  while (n--) { s += digits[*p >> 4]; s += digits[*p++ & 15]; } return s;
}
inline bool unhex(const String &s, uint8_t *out, unsigned n) {
  if (s.length() != n * 2) return false;
  for (unsigned i = 0; i < n * 2; ++i) {
    char c = s[i]; int v = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
    if (v < 0) return false;
    if (!(i & 1)) out[i / 2] = v << 4; else out[i / 2] |= v;
  } return true;
}
inline void derive(const String &password, const uint8_t salt[16], uint8_t result[32]) {
  const uint32_t started = micros();
  br_hmac_key_context key; br_hmac_key_init(&key, &br_sha256_vtable, password.c_str(), password.length());
  br_hmac_context ctx; uint8_t first[20], u[32]; memcpy(first, salt, 16); memset(first + 16, 0, 4); first[19] = 1;
  br_hmac_init(&ctx, &key, 32); br_hmac_update(&ctx, first, 20); br_hmac_out(&ctx, u); memcpy(result, u, 32);
  for (unsigned i = 1; i < Rounds; ++i) {
    br_hmac_init(&ctx, &key, 32); br_hmac_update(&ctx, u, 32); br_hmac_out(&ctx, u);
    for (unsigned j = 0; j < 32; ++j) result[j] ^= u[j];
    if (!(i & 31)) yield(); // Feed watchdog; this work never runs from rendering or polling.
  }
  lastDeriveMicros = micros() - started;
}
inline String passwordHash(const String &password) {
  uint8_t salt[16], value[32]; randomBytes(salt, 16); derive(password, salt, value);
  return "p2$" + hex(salt, 16) + "$" + hex(value, 32);
}
inline bool passwordValid(const String &password, const String &verifier) {
  if (!verifier.startsWith("p2$")) return verifier == "12345678" && password == verifier; // Unsaved factory default only.
  if (verifier.length() != 100 || verifier[35] != '$') return false;
  uint8_t salt[16], expected[32], actual[32];
  if (!unhex(verifier.substring(3, 35), salt, 16) || !unhex(verifier.substring(36), expected, 32)) return false;
  derive(password, salt, actual); return equal(expected, actual, 32);
}
inline bool key(uint8_t out[64], bool create) {
  File f = LittleFS.open("/config.key", "r");
  if (f) { bool ok = f.size() == 64 && f.read(out, 64) == 64; f.close(); return ok; }
  if (!create) return false;
  randomBytes(out, 64); f = LittleFS.open("/key.tmp", "w");
  if (!f) return false;
  bool ok = f.write(out, 64) == 64; f.flush(); f.close();
  if (!ok || !LittleFS.rename("/key.tmp", "/config.key")) { LittleFS.remove("/key.tmp"); return false; } return true;
}
class Writer : public Print {
  File &file; br_aes_ct_ctr_keys aes; br_hmac_key_context hk; br_hmac_context hm;
  uint8_t nonce[12], buffer[128]; unsigned used = 0; uint32_t counter = 0;
  bool ok = true;
  void flushBlock() {
    if (!used) return;
    counter = br_aes_ct_ctr_run(&aes, nonce, counter, buffer, used);
    br_hmac_update(&hm, buffer, used); ok &= file.write(buffer, used) == used; used = 0;
  }
public:
  Writer(File &f, const uint8_t key[64]) : file(f) {
    br_aes_ct_ctr_init(&aes, key, 32); randomBytes(nonce, 12);
    br_hmac_key_init(&hk, &br_sha256_vtable, key + 32, 32); br_hmac_init(&hm, &hk, 32);
    const uint8_t magic[8] = {'M','O','S','C','F','G','3',0};
    ok &= file.write(magic, 8) == 8; ok &= file.write(nonce, 12) == 12;
    br_hmac_update(&hm, magic, 8); br_hmac_update(&hm, nonce, 12);
  }
  size_t write(uint8_t byte) override { buffer[used++] = byte; if (used == sizeof buffer) flushBlock(); return ok ? 1 : 0; }
  bool finish() { flushBlock(); uint8_t tag[32]; br_hmac_out(&hm, tag); return ok && file.write(tag, 32) == 32; }
};
inline bool read(File &file, String &plain) {
  if (!file || file.size() < 52 || file.size() > 8244) return false;
  uint8_t keys[64], header[20], tag[32], actual[32]; if (!key(keys, false)) return false;
  if (file.read(header, 20) != 20 || memcmp(header, "MOSCFG3", 8)) return false;
  size_t length = file.size() - 52; plain.reserve(length);
  while (length--) { int c = file.read(); if (c < 0) return false; plain += char(c); }
  if (file.read(tag, 32) != 32) return false;
  br_hmac_key_context hk; br_hmac_context hm;
  br_hmac_key_init(&hk, &br_sha256_vtable, keys + 32, 32); br_hmac_init(&hm, &hk, 32);
  br_hmac_update(&hm, header, 20); br_hmac_update(&hm, plain.c_str(), plain.length()); br_hmac_out(&hm, actual);
  if (!equal(tag, actual, 32)) { plain = ""; return false; }
  br_aes_ct_ctr_keys aes; br_aes_ct_ctr_init(&aes, keys, 32);
  br_aes_ct_ctr_run(&aes, header + 8, 0, &plain[0], plain.length()); return true;
}
}
