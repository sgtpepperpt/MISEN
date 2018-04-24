#include "util.h"

void debug_print_points(float* points, size_t nr_rows, size_t row_len) {
    for (size_t y = 0; y < nr_rows; ++y) {
        for (size_t l = 0; l < row_len; ++l)
            outside_util::printf("%lf ", *(points + y * row_len + l));
        outside_util::printf("\n");
    }
}

void ok_response(uint8_t** out, size_t* out_len) {
    *out_len = 1;
    unsigned char* ret = (unsigned char*)malloc(sizeof(unsigned char));
    ret[0] = 'x';
    *out = ret;
}
