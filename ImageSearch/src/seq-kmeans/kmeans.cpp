#include "kmeans.h"

using namespace std;

void online_kmeans_init(float* descriptors, const size_t nr_descs, kmeans_data* data) {
    assertion(nr_descs >= data->k);

    memset(data->centres, 0x00, data->k * data->desc_len * sizeof(float));
    memset(data->counters, 0x00, data->k * sizeof(unsigned));

    //random select centres
    for(unsigned i = 0; i < data->k; i++) {
        memcpy(data->centres + i * data->desc_len, descriptors + i * data->desc_len, data->desc_len * sizeof(float));
    }

    data->started = 1;
}

double online_kmeans(float* descriptors, const size_t nr_descs, kmeans_data* data) {
    double compactness = 0;

    // for all available descriptors
    for (size_t i = 0; i < nr_descs; ++i) {
        float min_dist = DBL_MAX;
        int min_dist_centre = -1;

        // for all centres
        for(unsigned j = 0; j < data->k; j++) {
            float dist = normL2Sqr(data->centres + j * data->desc_len, descriptors + j * data->desc_len, data->desc_len);
            compactness += dist;

            if(dist < min_dist) {
                min_dist = dist;
                min_dist_centre = j;
            }
        }

        data->counters[min_dist_centre]++;

        // update the centre
        for(unsigned j = 0; j < data->desc_len; j++) {
            float* curr_val = data->centres + min_dist_centre * data->desc_len + j;
            *curr_val = *curr_val + (1 / data->counters[min_dist_centre]) * (descriptors[min_dist_centre * data->desc_len + j] - *curr_val);
        }
    }

    untrusted_util::printf("compactness %f\n", compactness);
    return 1;//best_compactness;
}
