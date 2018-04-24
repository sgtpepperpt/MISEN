#ifndef SGX_IMAGE_SEARCH_UTIL_H
#define SGX_IMAGE_SEARCH_UTIL_H

#include <stdlib.h>
#include <stdint.h>

#include "outside_util.h"

void debug_print_points(float* points, size_t nr_rows, size_t row_len);
void ok_response(uint8_t** out, size_t* out_len);

#endif
