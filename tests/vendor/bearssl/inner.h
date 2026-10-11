// DES-only host adapter for the unmodified BearSSL cipher sources.
#pragma once
#include <string.h>
#include "bearssl_block.h"
#define PROGMEM
#define pgm_read_byte(p) (*(const unsigned char *)(p))
static inline uint32_t br_dec32be(const void *p) { const unsigned char *b=p; return ((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3]; }
static inline void br_enc32be(void *p,uint32_t v) {unsigned char *b=p; b[0]=v>>24;b[1]=v>>16;b[2]=v>>8;b[3]=v;}
void br_des_do_IP(uint32_t *xl, uint32_t *xr);

/*
 * Apply DES Final Permutation (inverse of IP).
 */
void br_des_do_invIP(uint32_t *xl, uint32_t *xr);

/*
 * Key schedule unit: for a DES key (8 bytes), compute 16 subkeys. Each
 * subkey is two 28-bit words represented as two 32-bit words; the PC-2
 * bit extration is NOT applied.
 */
void br_des_keysched_unit(uint32_t *skey, const void *key);

/*
 * Reversal of 16 DES sub-keys (for decryption).
 */
void br_des_rev_skey(uint32_t *skey);

/*
 * DES/3DES key schedule for 'des_tab' (encryption direction). Returned
 * value is the number of rounds.
 */
unsigned br_des_tab_keysched(uint32_t *skey, const void *key, size_t key_len);

/*
 * DES/3DES key schedule for 'des_ct' (encryption direction). Returned
 * value is the number of rounds.
 */
unsigned br_des_ct_keysched(uint32_t *skey, const void *key, size_t key_len);

/*
 * DES/3DES subkey decompression (from the compressed bitsliced subkeys).
 */
void br_des_ct_skey_expand(uint32_t *sk_exp,
	unsigned num_rounds, const uint32_t *skey);

/*
 * DES/3DES block encryption/decryption ('des_tab').
 */
void br_des_tab_process_block(unsigned num_rounds,
	const uint32_t *skey, void *block);

/*
 * DES/3DES block encryption/decryption ('des_ct').
 */
void br_des_ct_process_block(unsigned num_rounds,
	const uint32_t *skey, void *block);


#include "bearssl_hash.h"
static inline void br_enc64be(void *p,uint64_t v) {br_enc32be(p,v>>32);br_enc32be((unsigned char *)p+4,v);}
static inline void br_range_dec32be(uint32_t *v,size_t n,const void *p){for(size_t i=0;i<n;i++)v[i]=br_dec32be((const unsigned char *)p+4*i);}
static inline void br_range_enc32be(void *p,const uint32_t *v,size_t n){for(size_t i=0;i<n;i++)br_enc32be((unsigned char *)p+4*i,v[i]);}
static inline uint32_t br_dec32le(const void *p){const unsigned char *b=p;return ((uint32_t)b[3]<<24)|((uint32_t)b[2]<<16)|((uint32_t)b[1]<<8)|b[0];}
static inline void br_enc32le(void *p,uint32_t v){unsigned char *b=p;b[0]=v;b[1]=v>>8;b[2]=v>>16;b[3]=v>>24;}
void br_aes_ct_bitslice_Sbox(uint32_t *q);
void br_aes_ct_ortho(uint32_t *q);
unsigned br_aes_ct_keysched(uint32_t *skey,const void *key,size_t key_len);
void br_aes_ct_skey_expand(uint32_t *skey,unsigned num_rounds,const uint32_t *comp_skey);
void br_aes_ct_bitslice_encrypt(unsigned num_rounds,const uint32_t *skey,uint32_t *q);
