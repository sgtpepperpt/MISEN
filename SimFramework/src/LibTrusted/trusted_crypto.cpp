#include "trusted_crypto.h"

#include <string.h>

int sodium_encrypt(
        unsigned char *ciphertext, // message_len + C_EXPBYTES
        const unsigned char *message, // message_len
        unsigned long long message_len,
        const unsigned char *nonce, // C_NONCESIZE
        const unsigned char *key // C_KEYSIZE
) {
    return crypto_secretbox_easy(ciphertext+crypto_secretbox_NONCEBYTES, message, message_len, nonce, key);
}

int sodium_decrypt(
        unsigned char *decrypted, // ciphertext_len - C_EXPBYTES
        const unsigned char *ciphertext, // ciphertext_len
        unsigned long long ciphertext_len,
        const unsigned char *nonce, // C_NONCESIZE
        const unsigned char *key // C_KEYSIZE
) {
    return crypto_secretbox_open_easy(decrypted, ciphertext+crypto_secretbox_NONCEBYTES, ciphertext_len-crypto_secretbox_NONCEBYTES, nonce, key);
}

void tcrypto::random(void* out, size_t len) {
    randombytes_buf(out, len);
}

unsigned tcrypto::random_uint() {
    return randombytes_random();
}

unsigned tcrypto::random_uint_range(unsigned min, unsigned max) {
    if (max < min)
        return max + (tcrypto::random_uint() % (unsigned) (min - max));

    return min + (tcrypto::random_uint() % (unsigned) (max - min));
}

int tcrypto::sha256(unsigned char *out, const unsigned char *in, size_t len) {
    return crypto_hash_sha256(out, in, len);
}

int tcrypto::hmac_sha256(void* out, const void* in, const size_t in_len, const void* key, const size_t key_len) {
    unsigned char hmac_key[SHA256_BLOCK_SIZE];
    memset(hmac_key, 0x00, SHA256_BLOCK_SIZE);

    if(key_len == SHA256_BLOCK_SIZE) {
        memcpy(hmac_key, key, key_len);
    } else if(key_len > SHA256_BLOCK_SIZE) {
        if(tcrypto::sha256(hmac_key, (unsigned char*)key, key_len))
            return 1;
    } else {
        memcpy(hmac_key, key, key_len); // includes the padding with zeros
    }

    unsigned char* first_hash_input = (unsigned char*)malloc(SHA256_BLOCK_SIZE + in_len);
    memcpy(first_hash_input + SHA256_BLOCK_SIZE, in, in_len);

    unsigned char second_hash_input[SHA256_BLOCK_SIZE + SHA256_OUTPUT_SIZE];

    // produce opad and ipad with keys, and store in hash inputs
    for (unsigned i = 0; i < SHA256_BLOCK_SIZE; ++i) {
        first_hash_input[i] = hmac_key[i] ^ 0x36; // i_key_pad
        second_hash_input[i] = hmac_key[i] ^ 0x5c; // o_key_pad
    }

    // store first hash directly in second hash input
    if(tcrypto::sha256(second_hash_input + SHA256_BLOCK_SIZE, first_hash_input, SHA256_BLOCK_SIZE + in_len))
        return 1;

    if(tcrypto::sha256((unsigned char*)out, second_hash_input, SHA256_OUTPUT_SIZE + SHA256_BLOCK_SIZE))
        return 1;

    // erase key
    memset(hmac_key, 0x00, SHA256_BLOCK_SIZE);
    memset(first_hash_input, 0x00, SHA256_BLOCK_SIZE + in_len);
    free(first_hash_input);

    return 0;
}

int tcrypto::encrypt(void* out, const unsigned char *in, const size_t in_len, const void* key, unsigned char* ctr) {
    uint8_t nonce[crypto_secretbox_NONCEBYTES];
    memset(nonce, 0x00, crypto_secretbox_NONCEBYTES);

    return sodium_encrypt((uint8_t*)out, in, in_len, nonce, (uint8_t*)key);
}

int tcrypto::decrypt(void* out, const unsigned char *in, const size_t in_len, const void* key, unsigned char* ctr) {
    uint8_t nonce[crypto_secretbox_NONCEBYTES];
    memset(nonce, 0x00, crypto_secretbox_NONCEBYTES);

    return sodium_decrypt((uint8_t*)out, in, in_len, nonce, (uint8_t*)key);
}
