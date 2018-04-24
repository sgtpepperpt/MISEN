#ifndef __PARALLEL_H
#define __PARALLEL_H

#include <stdlib.h>

#define PARALLEL_KMEANS 1
#if PARALLEL_KMEANS
typedef struct dist_res {
    double min_dist;
    size_t min_dist_centre;
    double compactness;
} dist_res;

typedef struct dist_args {
    size_t desc_len;
    float* centres;
    float* descriptors;
    size_t start;
    size_t end;

    dist_res* res;
} dist_args;
#endif

#define PARALLEL_IMG_PROCESSING 1
#if PARALLEL_IMG_PROCESSING
typedef struct process_args {
    size_t start;
    size_t end;
    float* descriptors;
    unsigned* frequencies;
} process_args;
#endif

#define PARALLEL_ADD_IMG 1
#if PARALLEL_ADD_IMG
typedef struct img_add_args {
    unsigned tid;
    unsigned long id;
    unsigned* frequencies;
    size_t centre_pos_start;
    size_t centre_pos_end;
} img_add_args;
#endif

#endif
