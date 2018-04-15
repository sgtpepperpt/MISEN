#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <stdlib.h>

// wrappers for ocalls
#include "ocall_wrapper.h"
#include "crypto.h"

extern void trusted_process_message(void** out, size_t* out_len, const void* in, const size_t in_len);

#if defined(__cplusplus)
extern "C" {
#endif

void ecall_process(void** out, size_t* out_len, const void* in, const size_t in_len);

#if defined(__cplusplus)
}
#endif

#endif
