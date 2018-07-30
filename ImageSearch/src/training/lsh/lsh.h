#ifndef VISEN_LSH_H
#define VISEN_LSH_H

#include <stdlib.h>
#include <vector>

using namespace std;

float** init_lsh(size_t centroids);
const unsigned* calc_freq(float** gaussians, float* descriptors, size_t nr_descs, int centroids);

#endif
