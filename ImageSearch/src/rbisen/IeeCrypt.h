#ifndef rbisenIeeCrypt_h
#define rbisenIeeCrypt_h

#include <sodium.h>

#define C_KEYSIZE   crypto_secretbox_KEYBYTES
#define C_NONCESIZE crypto_secretbox_NONCEBYTES
#define C_EXPBYTES (crypto_secretbox_NONCEBYTES+crypto_secretbox_MACBYTES)

#define H_KEYSIZE crypto_auth_hmacsha256_KEYBYTES
#define H_BYTES crypto_auth_hmacsha256_BYTES

void setKeys(unsigned char* new_kEnc, unsigned char* new_kF);

void destroyKeys();

unsigned char* get_kF();

unsigned char* get_kEnc();

// RANDOM FUNCTIONS
unsigned int c_random_uint();

unsigned int c_random_uint_range(int min, int max);

// CRYPTO FUNCTIONS
// -1 failure, 0 ok

int rbisen_c_encrypt(unsigned char* ciphertext, // message_len + C_EXPBYTES
                     const unsigned char* message, // message_len
                     unsigned long long message_len, const unsigned char* nonce, // C_NONCESIZE
                     const unsigned char* key // C_KEYSIZE
);

int rbisen_c_decrypt(unsigned char* decrypted, // ciphertext_len - C_EXPBYTES
                     const unsigned char* ciphertext, // ciphertext_len
                     unsigned long long ciphertext_len, const unsigned char* nonce, // C_NONCESIZE
                     const unsigned char* key // C_KEYSIZE
);

int rbisen_c_hmac(unsigned char* out, // H_BYTES
                  const unsigned char* in, // inlen
                  unsigned long long inlen, const unsigned char* k // H_KEYSIZE
);

int c_hmac_verify(const unsigned char* h, // H_BYTES
                  const unsigned char* in, // inlen
                  unsigned long long inlen, const unsigned char* k // H_KEYSIZE
);

#endif /* IeeCrypt_h */
