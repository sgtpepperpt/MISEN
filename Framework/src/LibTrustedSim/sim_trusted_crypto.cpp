#include "trusted_crypto.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void tcrypto::random(void* out, size_t len) {
    printf("Unimplemented in trustedlib simulation!\n");
    exit(1);
}

unsigned tcrypto::random_uint() {
    printf("Unimplemented in trustedlib simulation!\n");
    exit(1);
}

unsigned tcrypto::random_uint_range(unsigned min, unsigned max) {
    printf("Unimplemented in trustedlib simulation!\n");
    exit(1);
}

int tcrypto::sha256(unsigned char *out, const unsigned char *in, size_t len) {
    printf("Unimplemented in trustedlib simulation!\n");
    exit(1);
}

int tcrypto::hmac_sha256(void* out, const void* in, const size_t in_len, const void* key, const size_t key_len) {
    printf("Unimplemented in trustedlib simulation!\n");
    exit(1);
}

int tcrypto::encrypt(void* out, const unsigned char *in, const size_t in_len, const void* key, unsigned char* ctr) {
    printf("Unimplemented in trustedlib simulation!\n");
    exit(1);
}

int tcrypto::decrypt(void* out, const unsigned char *in, const size_t in_len, const void* key, unsigned char* ctr) {
    printf("Unimplemented in trustedlib simulation!\n");
    exit(1);
}

