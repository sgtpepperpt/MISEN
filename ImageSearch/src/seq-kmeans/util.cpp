#include "util.h"

float calc_distance(const float* a, const float* b, size_t n) {
    float s = 0.f;
    for(size_t i = 0; i < n; i++)  {
        float v = a[i] - b[i];
        s += v*v;
    }
    return s;
}
