#ifndef _UT_CRYPTO__H_
#define _UT_CRYPTO__H_

#include <stdlib.h>
#include <stdint.h>

#define SHA256_OUTPUT_SIZE 32
#define SHA256_BLOCK_SIZE 64

#define AES_KEY_SIZE 16
#define AES_BLOCK_SIZE 16

namespace utcrypto {
    void encrypt(uint8_t* out, const uint8_t* in, const size_t in_len, const uint8_t* key, uint8_t* ctr);
    void decrypt(uint8_t* out, const uint8_t* in, const size_t in_len, const uint8_t* key, uint8_t* ctr);
}

#endif
