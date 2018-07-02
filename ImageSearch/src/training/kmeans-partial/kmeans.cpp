#include "kmeans.h"
#include "sgx_tprotected_fs.h"

#include "trusted_crypto.h"
#include <vector>
#include <tuple>
#include <string>

using namespace std;

/*
k-means centre initialisation using the following algorithm:
Arthur & Vassilvitskii (2007) k-means++: The Advantages of Careful Seeding
*/
static void generateCentersPP(const int socket, const size_t nr_rows, float* _out_centres, const size_t row_len, const unsigned nr_centres) {
    const int NR_TRIALS = 3;
    int * _centres = (int*)malloc(nr_centres * sizeof(int));
    float *_dist = (float*)malloc(nr_rows * 3 * sizeof(float));

    float* dist = &_dist[0];
    float* tdist = dist + nr_rows;
    float* tdist2 = tdist + nr_rows;
    double sum0 = 0;

    int* centres = &_centres[0];
    int val;
    tcrypto::random((unsigned char *) &val, sizeof(int));
    centres[0] = (unsigned)val % nr_rows;

    /*for (unsigned i = 0; i < nr_rows; i++) {
        dist[i] = normL2Sqr(ndata + (i * row_len), ndata + (centres[0] * row_len), row_len);
        sum0 += dist[i];
    } */

    for (unsigned i = 0; i < nr_rows; i++) {
        /*uint8_t req[1+ sizeof(unsigned)];
        req[0] = '4';
        memcpy(req + 1, &i, sizeof(unsigned));
        size_t res_l;

        float* buffer_row;
        outside_util::uee_process(socket, (void**)&buffer_row, &res_l, req, 1+ sizeof(size_t));

        memcpy(req + 1, &centres[0], sizeof(unsigned));
        float* buffer_row2;
        outside_util::uee_process(socket, (void**)&buffer_row2, &res_l, req, 1+ sizeof(size_t));*/

        float* buffer_row = outside_util::get(i);
        float* buffer_row2 = outside_util::get(centres[0]);

        dist[i] = normL2Sqr(buffer_row, buffer_row2, row_len);
        sum0 += dist[i];

        //outside_util::printf("will try free\n");

        //outside_util::outside_free(buffer_row);
        //outside_util::outside_free(buffer_row2);
    }

    for (unsigned k = 1; k < nr_centres; k++) {
        double best_sum = DBL_MAX;
        int best_centre = -1;

        for (unsigned j = 0; j < NR_TRIALS; j++) {
            double val;
            tcrypto::random((unsigned char *) &val, sizeof(double));
            double p = (double)val*sum0;
            unsigned ci = 0;
            for (; ci < nr_rows - 1; ci++) {
                p -= dist[ci];
                if (p <= 0)
                    break;
            }

            KMeansPPDistanceComputer(socket, tdist2, dist, ci, row_len).compute(0, nr_rows);
            double s = 0;
            for (unsigned i = 0; i < nr_rows; i++)
                s += tdist2[i];

            if (s < best_sum) {
                best_sum = s;
                best_centre = ci;
                swap(tdist, tdist2);
            }
        }

        centres[k] = best_centre;
        sum0 = best_sum;
        swap(dist, tdist);
    }

    /*for (unsigned k = 0; k < nr_centres; k++) {
        const float* src = ndata + (centres[k] * row_len);
        float* dst = _out_centres + k * row_len;
        memcpy(dst, src, row_len * sizeof(float));
        //for (unsigned j = 0; j < row_len; j++)
        //    dst[j] = src[j];
    }*/

    for (unsigned k = 0; k < nr_centres; k++) {
        /*uint8_t req[1+ sizeof(unsigned)];
        req[0] = '4';
        memcpy(req + 1, &centres[k], sizeof(unsigned));
        size_t res_l;

        float* buffer_row;
        outside_util::uee_process(socket, (void**)&buffer_row, &res_l, req, 1+ sizeof(size_t));
*/

        float* buffer_row = outside_util::get(centres[k]);
        const float* src = buffer_row;
        float* dst = _out_centres + k * row_len;
        memcpy(dst, src, row_len * sizeof(float));
        //for (unsigned j = 0; j < row_len; j++)
        //    dst[j] = src[j];

       // outside_util::outside_free(buffer_row);
    }

    free(_centres);
    free(_dist);
}

double kmeans(const int socket, const size_t nr_rows, const size_t row_len, const unsigned nr_centres, int* nlabels, unsigned attempts, float* _centres) {
    assertion(nr_centres > 0);
    assertion(nr_rows >= nr_centres);

    // buffers for centres
    float *c = (float*)malloc(row_len * nr_centres * sizeof(float));
    float *oc = (float*)malloc(row_len * nr_centres * sizeof(float));

    float* centres = c;
    float* old_centres = oc;
    float temp[row_len];

    int *counters = (int*)malloc(nr_centres * sizeof(int));
    double *dists = (double*)malloc(sizeof(double) * nr_rows);

    const double epsilon = FLT_EPSILON * FLT_EPSILON;
    int max_count = 100;

    if (nr_centres == 1) {
        attempts = 1;
        max_count = 2;
    }

    double best_compactness = DBL_MAX;
    for (unsigned a = 0; a < attempts; a++) {
        double compactness = 0;

        for (int iter = 0; ;) {
            outside_util::printf("iteration %d\n", iter);
            double max_centre_shift = iter == 0 ? DBL_MAX : 0.0;
            swap(centres, old_centres);

            if (iter == 0) {
                generateCentersPP(socket, nr_rows, centres, row_len, nr_centres);
            } else {
                // compute centres
                memset(centres, 0x00, row_len * nr_centres * sizeof(float));
                memset(counters, 0x00, nr_centres * sizeof(int));

                for (unsigned i = 0; i < nr_rows; i++) {
                    /*uint8_t req[1+ sizeof(unsigned)];
                    req[0] = '4';
                    memcpy(req + 1, &i, sizeof(unsigned));
                    size_t res_l;

                    float* buffer_row;
                    outside_util::uee_process(socket, (void**)&buffer_row, &res_l, req, 1+ sizeof(unsigned));*/

                    float* buffer_row = outside_util::get(i);

                    const float* sample = buffer_row;
                    int k = nlabels[i];
                    float* centre = centres + k * row_len;
                    for (unsigned j = 0; j < row_len; j++)
                        centre[j] += sample[j];

                    counters[k]++;

                    outside_util::outside_free(buffer_row);
                }

                for (unsigned k = 0; k < nr_centres; k++) {
                    if (counters[k] != 0)
                        continue;

                    // if some cluster appeared to be empty then:
                    //   1. find the biggest cluster
                    //   2. find the farthest from the center point in the biggest cluster
                    //   3. exclude the farthest point from the biggest cluster and form a new 1-point cluster.
                    int max_k = 0;
                    for (unsigned k1 = 1; k1 < nr_centres; k1++) {
                        if (counters[max_k] < counters[k1])
                            max_k = k1;
                    }

                    double max_dist = 0;
                    int farthest_i = -1;
                    float* base_centre = centres + max_k * row_len;
                    float* _base_center = temp; // normalized
                    const float scale = 1.f/counters[max_k];
                    for (unsigned j = 0; j < row_len; j++)
                        _base_center[j] = base_centre[j] * scale;

                    for (unsigned i = 0; i < nr_rows; i++) {
                        if (nlabels[i] != max_k)
                            continue;

                        /*uint8_t req[1+ sizeof(unsigned)];
                        req[0] = '4';
                        memcpy(req + 1, &i, sizeof(unsigned));
                        size_t res_l;

                        float* buffer_row;
                        outside_util::uee_process(socket, (void**)&buffer_row, &res_l, req, 1+ sizeof(unsigned));*/

                        float* buffer_row = outside_util::get(i);

                        const float* sample = buffer_row;
                        double dist = normL2Sqr(sample, _base_center, row_len);
                        outside_util::outside_free(buffer_row);

                        if (max_dist <= dist) {
                            max_dist = dist;
                            farthest_i = i;
                        }
                    }

                    counters[max_k]--;
                    counters[k]++;
                    nlabels[farthest_i] = k;

                    /*uint8_t req[1+ sizeof(unsigned)];
                    req[0] = '4';
                    memcpy(req + 1, &farthest_i, sizeof(unsigned));
                    size_t res_l;

                    float* buffer_row;
                    outside_util::uee_process(socket, (void**)&buffer_row, &res_l, req, 1+ sizeof(unsigned));*/

                    float* buffer_row = outside_util::get(farthest_i);

                    const float* sample = buffer_row;
                    float* cur_centre = centres + k * row_len;
                    for (unsigned j = 0; j < row_len; j++) {
                        base_centre[j] -= sample[j];
                        cur_centre[j] += sample[j];
                    }

                    //outside_util::outside_free(buffer_row);
                }

                for (unsigned k = 0; k < nr_centres; k++) {
                    float* centre = centres + k * row_len;
                    assertion(counters[k] != 0);

                    const float scale = 1.f/counters[k];
                    for (unsigned j = 0; j < row_len; j++)
                        centre[j] *= scale;

                    if (iter > 0) {
                        double dist = 0;
                        const float* old_center = old_centres + k * row_len;
                        for (unsigned j = 0; j < row_len; j++) {
                            double t = centre[j] - old_center[j];
                            dist += t*t;
                        }
                        max_centre_shift = max(max_centre_shift, dist);
                    }
                }
            }

            const int is_last_iter = (++iter == max(max_count, 2) || max_centre_shift <= epsilon);

            if(is_last_iter) {
                // don't re-assign labels to avoid creation of empty clusters
                KMeansDistanceComputer(socket, dists, nlabels, centres, true, row_len, nr_centres).compute(0, nr_rows);

                // calc compactness by summing all dists
                compactness = 0;
                for(unsigned i = 0; i < nr_rows; i++ )
                    compactness += dists[i];

                break;
            } else {
                // assign labels
                KMeansDistanceComputer(socket, dists, nlabels, centres, false, row_len, nr_centres).compute(0, nr_rows);
            }
        }

        if (compactness < best_compactness) {
            best_compactness = compactness;

            for (unsigned i = 0; i < nr_centres; ++i) {
                memcpy(_centres + i * row_len, centres + i * row_len, row_len * sizeof(float));
                /*for (unsigned j = 0; j < row_len; ++j) {
                    memcpy(_centres + i * row_len + j, centres + i * row_len + j, sizeof(float));
                    //printf("%f ", *(_centres + i * row_len + j));
                }*/
            }//printf("\n");

            //_labels.copyTo(best_labels);
            int kkk = 0;
            for (unsigned i = 0; i < nr_rows; ++i){
                //memcpy(best_labels.data + i, nlabels + i, sizeof(int));
                kkk += *(nlabels + i);
            }
            outside_util::printf("sum nlabels %d\n", kkk);
        }
    }

    free(counters);
    free(c);
    free(oc);
    free(dists);
    outside_util::printf("compactness %f\n", best_compactness);
    return best_compactness;
}
