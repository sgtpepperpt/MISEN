#ifndef __TRUSTED_H_
#define __TRUSTED_H_

#include <stdlib.h>
#include <stdint.h>

void trusted_process_message(uint8_t** out, size_t* out_len, const uint8_t* in, const size_t in_len);

#endif
