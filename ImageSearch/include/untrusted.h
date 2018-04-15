#ifndef __UNTRUSTED_H_
#define __UNTRUSTED_H_

#include <stdlib.h>

void untrusted_process_message(void** out, size_t* out_len, const void* in, const size_t in_len);

#endif
