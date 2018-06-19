#ifndef K_KMEANS_H
#define K_KMEANS_H

#include "sgx_tprotected_fs.h"


#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <vector>
#include <tuple>
#include <string>

#include "util.h"
#include "trusted_crypto.h"
#include "outside_util.h"

using namespace std;

class KMeansPPDistanceComputer {
public:
    KMeansPPDistanceComputer(const int _socket, float *tdist2_, float *dist_, int ci_, const size_t _row_len) {
        tdist2 = tdist2_;
        dist = dist_;
        ci = ci_;
        row_len = _row_len;
        socket = _socket;
    }

    void compute(const int begin, const int end) { // TODO use these
        /*for (int i = begin; i < end; i++) {
            tdist2[i] = min(normL2Sqr(ndata + (i * row_len), ndata + (ci * row_len), row_len), dist[i]);
        }*/

        for (int i = begin; i < end; i++) {
            /*uint8_t req[1+ sizeof(unsigned)];
            req[0] = '4';
            memcpy(req + 1, &i, sizeof(unsigned));
            size_t res_l;

            float* buffer_row;
            outside_util::uee_process(socket, (void**)&buffer_row, &res_l, req, 1+ sizeof(unsigned));

            memcpy(req + 1, &ci, sizeof(unsigned));
            float* buffer_row2;
            outside_util::uee_process(socket, (void**)&buffer_row2, &res_l, req, 1+ sizeof(unsigned));*/


            float* buffer_row = outside_util::get(i);
            float* buffer_row2 = outside_util::get(ci);

            tdist2[i] = min(normL2Sqr(buffer_row, buffer_row2, row_len), dist[i]);

            //outside_util::outside_free(buffer_row);
            //outside_util::outside_free(buffer_row2);
        }
    }

private:
    float *tdist2;
    float *dist;
    int ci;
    size_t row_len;
    int socket;
};

class KMeansDistanceComputer {
public:
    KMeansDistanceComputer(const int socket_, double *distances_, int *labels_, float *centers_, const bool _only_distance, const size_t _row_len, const unsigned _nr_centres) {
        distances = distances_;
        labels = labels_;
        centres = centers_;
        only_distance = _only_distance;
        row_len = _row_len;
        nr_centres = _nr_centres;
        socket = socket_;
    }

    void compute(int begin, int end) {
        for (int i = begin; i < end; ++i) {
            /*uint8_t req[1+ sizeof(unsigned)];
            req[0] = '4';
            memcpy(req + 1, &i, sizeof(unsigned));
            size_t res_l;

            float* buffer_row;
            outside_util::uee_process(socket, (void**)&buffer_row, &res_l, req, 1+ sizeof(unsigned));*/
            float* buffer_row = outside_util::get(i);

            const float* sample = buffer_row;
            if (only_distance) {
                const float *centre = centres + labels[i] * row_len;
                distances[i] = normL2Sqr(sample, centre, row_len);
                continue;
            } else {
                int k_best = 0;
                double min_dist = DBL_MAX;

                for (unsigned k = 0; k < nr_centres; k++) {
                    const float* centre = centres + k * row_len;
                    const double dist = normL2Sqr(sample, centre, row_len);

                    if (min_dist > dist) {
                        min_dist = dist;
                        k_best = k;
                    }
                }

                distances[i] = min_dist;
                labels[i] = k_best;
            }

           // outside_util::outside_free(buffer_row);
        }
    }

private:
    double* distances;
    int* labels;
    float* centres;
    bool only_distance;
    size_t row_len;
    unsigned nr_centres;
    int socket;
};

double kmeans(const int socket, const size_t nr_rows, const size_t row_len, const unsigned nr_centres, int *nlabels, unsigned attempts, float *_centres);

#endif
