#include "util.h"

float normL2Sqr(const float* a, const float* b, size_t n) {
    float s = 0.f;
    for(size_t i = 0; i < n; i++)  {
        float v = a[i] - b[i];
        s += v*v;
    }
    return s;
}

#include "types.h"
long util_time_elapsed(untrusted_time start, untrusted_time end) {
    long secs_used, micros_used;

    secs_used = (end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
    micros_used = ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
    return micros_used;
}
