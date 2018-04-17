#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <openssl/aes.h>
#include <openssl/rand.h>

void c_encrypt(uint8_t* out, const uint8_t*in, const size_t in_len, const uint8_t* key, uint8_t* ctr);
void c_decrypt(uint8_t* out, const uint8_t*in, const size_t in_len, const uint8_t* key, uint8_t* ctr);
