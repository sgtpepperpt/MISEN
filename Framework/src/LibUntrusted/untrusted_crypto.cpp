#include "untrusted_crypto.h"

#include <stdlib.h>
#include <stdio.h> // for debug purposes
#include <stdint.h>
#include <string.h>

#include <openssl/aes.h>
#include <openssl/rand.h>

void utcrypto::encrypt(uint8_t* out, const uint8_t* in, const size_t in_len, const uint8_t* key, uint8_t* ctr) {
    /*AES_KEY aes_key;
    if(AES_set_encrypt_key(key, 128, &aes_key)) {
        printf("AES_set_encrypt_key error.\n");
        exit(-1);
    }

    unsigned num = 0;
    unsigned char ecount[AES_BLOCK_SIZE];
    memset(ecount, 0x00, AES_BLOCK_SIZE);

    AES_ctr128_encrypt(in, out, in_len, &aes_key, ctr, ecount, &num);*/
}

void utcrypto::decrypt(uint8_t* out, const uint8_t* in, const size_t in_len, const uint8_t* key, uint8_t* ctr) {
    /*AES_KEY aes_key;
    if (AES_set_encrypt_key(key, 128, &aes_key)){
        printf("AES_set_encrypt_key error.\n");
        exit(-1);
    }

    unsigned num = 0;
    unsigned char ecount[AES_BLOCK_SIZE];
    memset(ecount, 0x00, AES_BLOCK_SIZE);

    AES_ctr128_encrypt(in, out, in_len, &aes_key, ctr, ecount, &num);*/
}


