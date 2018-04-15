#include "util.h"

float normL2Sqr(const float* a, const float* b, size_t n, unsigned batch_size) {
    float s = 0.f;
    for(size_t i = 0; i < n; i++)  {
        float v = a[i] - b[i];
        s += batch_size * v*v;
    }
    return s;
}

void swap(void* a, void* b) {
    void* tmp = a;
    a = b;
    b = tmp;
}
