#include "kmeans.h"

#include <string.h>
#include <float.h>

#include "untrusted_util.h"
#include "trusted_util.h"
#include "util.h"

#define PARALLEL_KMEANS 1

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

long time_elapsed(untrusted_time start, untrusted_time end) {
    long secs_used, micros_used;

    secs_used = (end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
    micros_used = ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
    return micros_used;
}

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

#if PARALLEL_KMEANS
void* calc_distances(void* args) {
    dist_args* arg = (dist_args*)args;
    dist_res* res = (dist_res*)malloc(sizeof(dist_res));
    res->min_dist = DBL_MAX;
    res->min_dist_centre = 0;
    res->compactness = 0;

    for(size_t j = arg->start; j < arg->end; j++) {
        float dist = normL2Sqr(arg->centres + j * arg->desc_len, arg->descriptors + j * arg->desc_len, arg->desc_len);
        res->compactness += dist;

        if(dist < res->min_dist) {
            res->min_dist = dist;
            res->min_dist_centre = j;
        }
    }

    arg->res = res;

    return res;//TODO return array of results from do_work?
}
#endif

double online_kmeans(float* descriptors, const size_t nr_descs, kmeans_data* data) {
    untrusted_time start = untrusted_util::curr_time();

    double compactness = 0;

#if PARALLEL_KMEANS
    // initialisation
    unsigned nr_threads = trusted_util::thread_get_count();
    size_t k_per_thread = data->k / nr_threads;

    dist_args* args = (dist_args*)malloc(nr_threads * sizeof(dist_args));
#endif

    // for all available descriptors
    for (size_t i = 0; i < nr_descs; ++i) {
        double min_dist = DBL_MAX;
        size_t min_dist_centre = 0;

#if PARALLEL_KMEANS
        for (unsigned j = 0; j < nr_threads; ++j) {
            // each thread receives the generic pointers and the thread ranges
            args[j].desc_len = data->desc_len;
            args[j].centres = data->centres;
            args[j].descriptors = descriptors;

            if(j == 0) {
                args[j].start = 0;
                args[j].end = j * k_per_thread + k_per_thread;
            } else {
                args[j].start = j * k_per_thread + 1;

                if (j + 1 == nr_threads)
                    args[j].end = data->k;
                else
                    args[j].end = j * k_per_thread + k_per_thread;
            }

            //untrusted_util::printf("start %lu end %lu\n", args[j].start, args[j].end);
            trusted_util::thread_add_work(calc_distances, args + j);
        }

        trusted_util::thread_do_work();

        // gather the results from each thread
        for (unsigned j = 0; j < nr_threads; ++j) {
            compactness += args[j].res->compactness;

            if(args[j].res->min_dist < min_dist) {
                min_dist = args[j].res->min_dist;
                min_dist_centre = args[j].res->min_dist_centre;
            }

            free(args[j].res);
        }
#else
        // for all centres
        for(unsigned j = 0; j < data->k; j++) {
            float dist = normL2Sqr(data->centres + j * data->desc_len, descriptors + j * data->desc_len, data->desc_len);
            compactness += dist;

            if(dist < min_dist) {
                min_dist = dist;
                min_dist_centre = j;
            }
        }
#endif
        data->counters[min_dist_centre]++;

        // update the centre
        for(unsigned j = 0; j < data->desc_len; j++) {
            float* curr_val = data->centres + min_dist_centre * data->desc_len + j;
            *curr_val = *curr_val + (1 / data->counters[min_dist_centre]) * (descriptors[min_dist_centre * data->desc_len + j] - *curr_val);
        }
    }

#if PARALLEL_KMEANS
    free(args);
#endif

    untrusted_time end = untrusted_util::curr_time();
    untrusted_util::printf("elapsed %ld\n", time_elapsed(start, end));

    untrusted_util::printf("compactness %f\n", compactness);
    return 1;//best_compactness;
}
