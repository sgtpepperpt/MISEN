#ifndef ONLINE_KMEANS_H
#define ONLINE_KMEANS_H

#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "../../../Framework/include/untrusted_util.h"

#include "util.h"

using namespace std;

typedef struct kmeans_data {
    size_t desc_len;
    size_t k;
    float* centres; // nr_clusters * desc_len
    unsigned *counters; // nr_clusters
    int started;
} kmeans_data;

void online_kmeans_init(float* descriptors, const size_t nr_descs, kmeans_data* data);
double online_kmeans(float* descriptors, const size_t nr_descs, kmeans_data* data);

#endif
