#include "img_processing.h"

#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "trusted_util.h"
#include "outside_util.h"

#include "parallel.h"
#include "seq-kmeans/util.h"

#if PARALLEL_IMG_PROCESSING
static void* parallel_process(void* args) {
    process_args* arg = (process_args*)args;

    for (size_t i = arg->start; i < arg->end; ++i) {
        double min_dist = DBL_MAX;
        int pos = -1;

        for (size_t j = 0; j < arg->nr_centres; ++j) {
            double dist_to_centre = calc_distance(arg->k->get_centre(j), arg->descriptors + i * arg->desc_len, arg->desc_len);

            if(dist_to_centre < min_dist) {
                min_dist = dist_to_centre;
                pos = j;
            }
        }

        arg->frequencies[pos]++;
    }

    return NULL;
}
#endif

const unsigned* process_new_image(BagOfWordsTrainer* k, const size_t nr_desc, float* descriptors) {
    untrusted_time start = outside_util::curr_time();

    unsigned* frequencies = (unsigned*)malloc(k->nr_centres() * sizeof(unsigned));
    memset(frequencies, 0x00, k->nr_centres() * sizeof(unsigned));

#if PARALLEL_IMG_PROCESSING
    // initialisation
    unsigned nr_threads = trusted_util::thread_get_count();
    size_t desc_per_thread = nr_desc / nr_threads;

    process_args* args = (process_args*)malloc(nr_threads * sizeof(process_args));

    for (unsigned j = 0; j < nr_threads; ++j) {
        // each thread receives the generic pointers and the thread ranges
        args[j].descriptors = descriptors;
        args[j].frequencies = frequencies;
        args[j].nr_centres = k->nr_centres();
        args[j].desc_len = k->desc_len();
        args[j].k = k;

        if(j == 0) {
            args[j].start = 0;
            args[j].end = j * desc_per_thread + desc_per_thread;
        } else {
            args[j].start = j * desc_per_thread + 1;

            if (j + 1 == nr_threads)
                args[j].end = nr_desc;
            else
                args[j].end = j * desc_per_thread + desc_per_thread;
        }

        //outside_util::printf("start %lu end %lu\n", args[j].start, args[j].end);
        trusted_util::thread_add_work(parallel_process, args + j);
    }

    trusted_util::thread_do_work();
    free(args);
#else
    // for each descriptor, calculate its closest centre
    for (size_t i = 0; i < nr_desc; ++i) {
        double min_dist = DBL_MAX;
        int pos = -1;

        for (size_t j = 0; j < k->nr_centres(); ++j) {
            double dist_to_centre = calc_distance(k->get_centre(j), descriptors + i * k->desc_len(), k->desc_len());

            if(dist_to_centre < min_dist) {
                min_dist = dist_to_centre;
                pos = j;
            }
        }

        frequencies[pos]++;
    }
#endif

    untrusted_time end = outside_util::curr_time();
    outside_util::printf("elapsed %ld\n", trusted_util::time_elapsed_ms(start, end));

    return frequencies;
}
