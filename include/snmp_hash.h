#pragma once
#include "snmp_md5.h"
#ifdef ESP8266
#include <bearssl/bearssl_hash.h>
#else
#include <bearssl_hash.h>
#endif
namespace SnmpCrypto {
// RFC 3414 uses the authentication hash for both authentication and privacy key localization.
class Hash {
  bool sha;
  Md5 md5;
  br_sha1_context sha1;

public:
  explicit Hash(bool useSha = false) : sha(useSha) {
    br_sha1_init(&sha1);
  }
  unsigned size() const {
    return sha ? 20 : 16;
  }
  void add(const void *data, size_t size) {
    if (sha)
      br_sha1_update(&sha1, data, size);
    else
      md5.add(static_cast<const uint8_t *>(data), size);
  }
  void finish(uint8_t *out) {
    if (sha)
      br_sha1_out(&sha1, out);
    else
      md5.finish(out);
  }
};
inline void authenticate(bool sha, const uint8_t *key, const uint8_t *data, size_t size,
                         uint8_t *out) {
  uint8_t pad[64], inner[20];
  unsigned keySize = sha ? 20 : 16;
  for (unsigned i = 0; i < 64; i++)
    pad[i] = (i < keySize ? key[i] : 0) ^ 0x36;
  Hash first(sha);
  first.add(pad, 64);
  first.add(data, size);
  first.finish(inner);
  for (unsigned i = 0; i < 64; i++)
    pad[i] = (i < keySize ? key[i] : 0) ^ 0x5c;
  Hash second(sha);
  second.add(pad, 64);
  second.add(inner, keySize);
  second.finish(out);
}
} // namespace SnmpCrypto
