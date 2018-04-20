#ifndef KMEANS_UTIL_H
#define KMEANS_UTIL_H

#include <stdlib.h>
#include "untrusted_util.h"

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

#define assertion(test) do{if(!(test)){untrusted_util::printf("Assertion failed!");untrusted_util::exit(1);}}while(0)

float normL2Sqr(const float* a, const float* b, size_t n);

#endif
