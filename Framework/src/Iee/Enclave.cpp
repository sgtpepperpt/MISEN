#include "Enclave.h"

void ecall_process(void** out, size_t* out_len, const void* in, const size_t in_len) {
    trusted_process_message(out, out_len, in, in_len);
}
