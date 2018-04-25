#ifndef KMEANS_UTIL_H
#define KMEANS_UTIL_H

#include <stdlib.h>
#include "outside_util.h"

#define assertion(test) do{if(!(test)){outside_util::printf("Assertion failed!");outside_util::exit(1);}}while(0)

float calc_distance(const float* a, const float* b, size_t n);

#endif
