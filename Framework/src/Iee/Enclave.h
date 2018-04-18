#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <stdlib.h>
#include <stdint.h>

// wrappers for ocalls
#include "ocall_wrapper.h"
#include "crypto.h"

extern void trusted_init();
extern void trusted_thread_enter();
extern void trusted_process_message(uint8_t** out, size_t* out_len, const uint8_t* in, const size_t in_len);

#if defined(__cplusplus)
extern "C" {
#endif

void ecall_init_enclave();
void ecall_thread_enter();
void ecall_process(void** out, size_t* out_len, const void* in, const size_t in_len);

#if defined(__cplusplus)
}
#endif

#endif
