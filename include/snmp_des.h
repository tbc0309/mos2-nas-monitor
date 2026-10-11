#pragma once
#ifdef ESP8266
#include <bearssl/bearssl_block.h>
#else
#include <bearssl_block.h>
#endif
namespace SnmpCrypto {
// RFC 3826 CFB128: AES always encrypts the feedback block, including when decrypting.
inline bool aes(bool encrypt, const uint8_t *key, uint8_t *iv, uint8_t *data, size_t size) {
  if (!size)
    return false;
  br_aes_ct_cbcenc_keys ctx;
  br_aes_ct_cbcenc_init(&ctx, key, 16);
  for (size_t pos = 0; pos < size; pos += 16) {
    uint8_t stream[16], zero[16] = {}, cipher[16] = {};
    memcpy(stream, iv, 16);
    br_aes_ct_cbcenc_run(&ctx, zero, stream, 16);
    size_t count = size - pos < 16 ? size - pos : 16;
    if (!encrypt)
      memcpy(cipher, data + pos, count);
    for (size_t i = 0; i < count; i++)
      data[pos + i] ^= stream[i];
    if (encrypt)
      memcpy(cipher, data + pos, count);
    memcpy(iv, cipher, count);
  }
  return true;
}
// Use the platform's constant-time DES primitive; callers authenticate before decrypting.
inline bool des(bool encrypt, const uint8_t *key, uint8_t *iv, uint8_t *data, size_t size) {
  if (!size || size % 8)
    return false;
  if (encrypt) {
    br_des_ct_cbcenc_keys ctx;
    br_des_ct_cbcenc_init(&ctx, key, 8);
    br_des_ct_cbcenc_run(&ctx, iv, data, size);
  } else {
    br_des_ct_cbcdec_keys ctx;
    br_des_ct_cbcdec_init(&ctx, key, 8);
    br_des_ct_cbcdec_run(&ctx, iv, data, size);
  }
  return true;
}
} // namespace SnmpCrypto
