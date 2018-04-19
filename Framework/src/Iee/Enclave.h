#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <stdlib.h>
#include <stdint.h>

// external lib should implement these
extern void trusted_init();
extern void trusted_thread_do_task(void* args);
extern void trusted_process_message(uint8_t** out, size_t* out_len, const uint8_t* in, const size_t in_len);

#if defined(__cplusplus)
extern "C" {
#endif

void ecall_init_enclave(unsigned nr_threads);
void ecall_thread_enter();
void ecall_process(void** out, size_t* out_len, const void* in, const size_t in_len);

#if defined(__cplusplus)
}
#endif

#endif
