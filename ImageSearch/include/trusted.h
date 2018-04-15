#ifndef __TRUSTED_H_
#define __TRUSTED_H_

#include <stdlib.h>

void trusted_process_message(void** out, size_t* out_len, const void* in, const size_t in_len);

#endif
