#ifndef W_UTIL_H
#define W_UTIL_H

#include <stdlib.h>
#include "ocall_wrapper.h"

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

#define assertion(test) do{if(!(test)){sgx_printf("Assertion failed!");sgx_exit(1);}}while(0)

float normL2Sqr(const float* a, const float* b, size_t n, unsigned batch_size);
void swap(void* a, void* b);

#endif
