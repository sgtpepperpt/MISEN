#ifndef _CRYPTO__H_
#define _CRYPTO__H_

#include <stdlib.h>

#define SHA256_OUTPUT_SIZE 32
#define SHA256_BLOCK_SIZE 64

#define AES_KEY_SIZE 16
#define AES_BLOCK_SIZE 16

void c_random(void* out, size_t len);
unsigned c_random_uint();
unsigned c_random_uint_range(unsigned min, unsigned max);

int c_sha256(unsigned char *out, const unsigned char *in, size_t len);
int c_hmac_sha256(void* out, const void* in, const size_t in_len, const void* key, const size_t key_len);

int c_encrypt(void* out, const unsigned char *in, const size_t in_len, const void* key, unsigned char* ctr);
int c_decrypt(void* out, const unsigned char *in, const size_t in_len, const void* key, unsigned char* ctr);

#endif
