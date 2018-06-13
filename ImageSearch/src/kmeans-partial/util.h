#ifndef K_UTIL_H
#define K_UTIL_H

#include <stdlib.h>
#include "outside_util.h"

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

#define assertion(test) do{if(!(test)){outside_util::printf("Assertion failed!");outside_util::exit(1);}}while(0)

float normL2Sqr(const float* a, const float* b, size_t n);
void swap(void* a, void* b);

#endif
