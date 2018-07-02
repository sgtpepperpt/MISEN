#ifndef ONLINE_KMEANS_H
#define ONLINE_KMEANS_H

#include <stdlib.h>

typedef struct kmeans_data {
    size_t desc_len;
    size_t k;
    float* centres; // nr_clusters * desc_len
    unsigned *counters; // nr_clusters
    int started;
} kmeans_data;

void train_kmeans_init(float* descriptors, const size_t nr_descs, kmeans_data* data);
double train_kmeans(float* descriptors, const size_t nr_descs, kmeans_data* data);

#endif
