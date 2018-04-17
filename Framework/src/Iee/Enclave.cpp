#include "Enclave.h"

#include <string.h>

void ecall_process(void** out, size_t* out_len, const void* in, const size_t in_len) {
    /*// TODO do not use this hardcoded values
    uint8_t key[AES_BLOCK_SIZE];
    memset(key, 0x00, AES_BLOCK_SIZE);

    uint8_t ctr[AES_BLOCK_SIZE];
    memset(ctr, 0x00, AES_BLOCK_SIZE);

    // decrypt input
    uint8_t* in_unenc = (uint8_t*)malloc(in_len);
    c_decrypt(in_unenc, (uint8_t *)in, in_len, key, ctr);

    // prepare unencrypted output
    uint8_t* out_unenc = NULL;
    trusted_process_message(&out_unenc, out_len, in_unenc, in_len);

    // encrypt output
    memset(ctr, 0x00, AES_BLOCK_SIZE);
    *out = sgx_untrusted_malloc(*out_len);
    c_encrypt(*out, out_unenc, *out_len, key, ctr);

    // cleanup
    free(in_unenc);
    free(out_unenc);*/

    trusted_process_message((uint8_t**)out, out_len, (const uint8_t*)in, in_len);

    // step needed to copy from unencrypted inside to encrypted outside
    uint8_t* res = (uint8_t*)sgx_untrusted_malloc(*out_len);
    memcpy(res, *out, *out_len);

    free(*out);

    *out = res;
}
