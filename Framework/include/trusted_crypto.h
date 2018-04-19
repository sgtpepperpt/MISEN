#ifndef _CRYPTO__H_
#define _CRYPTO__H_

#include <stdlib.h>

#define SHA256_OUTPUT_SIZE 32
#define SHA256_BLOCK_SIZE 64

#define AES_KEY_SIZE 16
#define AES_BLOCK_SIZE 16

namespace tcrypto {
    void random(void* out, size_t len);
    unsigned random_uint();
    unsigned random_uint_range(unsigned min, unsigned max);

    int sha256(unsigned char *out, const unsigned char *in, size_t len);
    int hmac_sha256(void* out, const void* in, const size_t in_len, const void* key, const size_t key_len);

    int encrypt(void* out, const unsigned char *in, const size_t in_len, const void* key, unsigned char* ctr);
    int decrypt(void* out, const unsigned char *in, const size_t in_len, const void* key, unsigned char* ctr);
}

#endif
