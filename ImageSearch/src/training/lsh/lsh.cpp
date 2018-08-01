#include "lsh.h"

#include "trusted_crypto.h"
#include "trusted_util.h"
#include "outside_util.h"

#include <random>
#include <map>
#include <fstream>
#include <float.h>
#include <math.h>
#include <string.h>

#define DESC_LEN 128

float dot_product(float* a, float* b) {
    float dot = 0;
    for (int j = 0; j < DESC_LEN; ++j) {
        dot += a[j] * b[j];
    }

    return dot;
}

float gaussrand(double mu, double sigma) {
    static float V1, V2, S;
    static int phase = 0;
    float X;

    if(phase == 0) {
        do {
            float U1 = (float)tcrypto::random_uint() / RAND_MAX;
            float U2 = (float)tcrypto::random_uint() / RAND_MAX;

            V1 = 2 * U1 - 1;
            V2 = 2 * U2 - 1;
            S = V1 * V1 + V2 * V2;
        } while(S >= 1 || S == 0);

        X = V1 * sqrt(-2 * log(S) / S);
    } else
        X = V2 * sqrt(-2 * log(S) / S);

    phase = 1 - phase;

    return X * sigma + mu;
}

typedef struct lsh_args {
    size_t start;
    size_t end;
    float* descriptors;
    unsigned* frequencies;

    size_t nr_centres;
    float** gaussians;
    float* clusters;
} lsh_args;

static void* parallel(void* args) {
    lsh_args* arg = (lsh_args*)args;

    //outside_util::printf("start %lu end %lu\n", arg->start, arg->end);
    for (size_t k = arg->start; k < arg->end; ++k) {
        double max = DBL_MIN;
        unsigned x = 0;
        for (unsigned i = 0; i < arg->nr_centres; ++i) {
            float d = dot_product(arg->gaussians[i], arg->descriptors + DESC_LEN * k);
            //printf("vector %d: %f\n", k, d);
            if(d > max) {
                max = d;
                x = i;
            }
        }
        //printf("chose %d\n", x);
        if(!arg->frequencies[x])
            arg->frequencies[x] = 1;
        else
            arg->frequencies[x]++;
    }

    return NULL;
}
lsh_args* args = NULL;
const unsigned* calc_freq(float** gaussians, float* descriptors, size_t nr_descs, int centroids) {
    unsigned* frequencies = (unsigned*)malloc(centroids * sizeof(unsigned));
    memset(frequencies, 0x00, centroids * sizeof(unsigned));

    // initialisation
    unsigned nr_threads = trusted_util::thread_get_count();

    size_t desc_per_thread = nr_descs / nr_threads;
    if(!args)
        args = (lsh_args*)malloc(nr_threads * sizeof(lsh_args));

    for (unsigned j = 0; j < nr_threads; ++j) {
        // each thread receives the generic pointers and the thread ranges
        args[j].descriptors = descriptors;
        args[j].frequencies = frequencies;
        args[j].gaussians = gaussians;
        args[j].nr_centres = centroids;

        if(j == 0) {
            args[j].start = 0;
            args[j].end = j * desc_per_thread + desc_per_thread;
        } else {
            args[j].start = j * desc_per_thread + 1;

            if (j + 1 == nr_threads)
                args[j].end = nr_descs;
            else
                args[j].end = j * desc_per_thread + desc_per_thread;
        }

        //outside_util::printf("start %lu end %lu\n", args[j].start, args[j].end);
        trusted_util::thread_add_work(parallel, args + j);
    }

    trusted_util::thread_do_work();
    //free(args);
/*
    for (int k = 0; k < nr_descs; ++k) {
        double max = DBL_MIN;
        int x = -1;
        for (int i = 0; i < centroids; ++i) {
            float d = dot_product(gaussians[i], descriptors + DESC_LEN * i);
            //printf("vector %d: %f\n", k, d);
            if(d > max) {
                max = d;
                x = i;
            }
        }
        //printf("chose %d\n", x);
        if(!frequencies[x])
            frequencies[x] = 1;
        else
            frequencies[x]++;
    }
*/
    return frequencies;
}

#include "outside_util.h"
float** init_lsh(size_t centroids) {
    float** gaussians = (float**)malloc(centroids * sizeof(float*));

    //std::default_random_engine generator(time(0));
    //std::normal_distribution<float> distribution(0.0, 25.0);

    for (unsigned i = 0; i < centroids; ++i) {
        gaussians[i] = (float*)malloc(DESC_LEN * sizeof(float));
        for (int j = 0; j < DESC_LEN; ++j)
            gaussians[i][j] = gaussrand(0.0, 25.0); //distribution(generator);

        for (size_t l = 0; l < DESC_LEN; ++l) {
            if(isnan(gaussians[i][l]))
                outside_util::printf("erro\n\n");

            //outside_util::printf("%f ", vec[l]);
        }
        //outside_util::printf("\n");
    }

    return gaussians;
}
