#ifndef W_KMEANS_H
#define W_KMEANS_H

#include <stdlib.h>
#include <string.h>
#include <float.h>

#include <sgx_trts.h>

#include "util.h"

using namespace std;

class KMeansPPDistanceComputer {
public:
    KMeansPPDistanceComputer(float *tdist2_, float *_ndata, float *dist_, int ci_, const size_t _row_len) {
        tdist2 = tdist2_;
        ndata = _ndata;
        dist = dist_;
        ci = ci_;
        row_len = _row_len;
    }

    void compute(int begin, int end, unsigned previous_count, unsigned batch_size) {
        for (int i = begin; i < previous_count; i++) {
            tdist2[i] = min(normL2Sqr(ndata + (i * row_len), ndata + (ci * row_len), row_len, batch_size), dist[i]);
        }

        for (int i = previous_count; i < end; i++) {
            tdist2[i] = min(normL2Sqr(ndata + (i * row_len), ndata + (ci * row_len), row_len, 1), dist[i]);
        }
    }

private:
    float *tdist2;
    float *dist;
    int ci;
    float *ndata;
    size_t row_len;
};

class KMeansDistanceComputer {
public:
    KMeansDistanceComputer(double *distances_, int *labels_, float *data_, float *centers_, const bool _only_distance, const size_t _row_len, const unsigned _nr_centres) {
        distances = distances_;
        labels = labels_;
        data = data_;
        centres = centers_;
        only_distance = _only_distance;
        row_len = _row_len;
        nr_centres = _nr_centres;
    }

    void compute(int begin, int end) {
        for (int i = begin; i < end; ++i) {
            const float *sample = data + (i * row_len);
            if (only_distance) {
                const float *centre = centres + labels[i] * row_len;
                distances[i] = normL2Sqr(sample, centre, row_len, 1);
                continue;
            } else {
                int k_best = 0;
                double min_dist = DBL_MAX;

                for (unsigned k = 0; k < nr_centres; k++) {
                    const float *centre = centres + k * row_len;
                    const double dist = normL2Sqr(sample, centre, row_len, 1);

                    if (min_dist > dist) {
                        min_dist = dist;
                        k_best = k;
                    }
                }

                distances[i] = min_dist;
                labels[i] = k_best;
            }
        }
    }

private:
    double *distances;
    int *labels;
    float *data;
    float *centres;
    bool only_distance;
    size_t row_len;
    unsigned nr_centres;
};

double seqkmeans(float *ndata, const size_t nr_rows, const size_t row_len, const unsigned nr_centres, int *nlabels, unsigned attempts, float *_centres, unsigned previous_count, unsigned batch_size);

#endif
