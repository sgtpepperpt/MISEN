#ifndef ONLINE_KMEANS_H
#define ONLINE_KMEANS_H

#include <stdlib.h>
#include "training/bagofwords.h"

void train_kmeans_init(float* descriptors, const size_t nr_descs, kmeans_data* data);
double train_kmeans(float* descriptors, const size_t nr_descs, kmeans_data* data);

#endif
