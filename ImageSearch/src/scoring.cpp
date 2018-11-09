#include "scoring.h"

#include <string.h>
#include <math.h>

#include "outside_util.h"

using namespace std;

double* idf = NULL;
double* calc_idf(size_t total_docs, unsigned* counters, const size_t nr_centres) {
    if(!idf)
        idf = (double*)malloc(nr_centres * sizeof(double));

    memset(idf, 0x00, nr_centres * sizeof(double));

    for(size_t i = 0; i < nr_centres; i++) {
        double non_zero_counters = counters[i] ? (double)counters[i] : (double)counters[i] + 1.0;
        idf[i] = log10((double)total_docs / non_zero_counters);
        //printf("%lu %lu %lf\n", total_docs, (size_t)non_zero_counters, log10((double)total_docs / non_zero_counters));
    }

    return idf;
}

void weight_idf(double *idf, unsigned* frequencies, const size_t nr_centres) {
    for (size_t i = 0; i < nr_centres; ++i)
        idf[i] *= frequencies[i];
}

/******************************************* SORT SCORED RESULTS *******************************************/
/*static */int compare_results(const void *a, const void *b) {
    double d_a = *((const double*) ((const uint8_t *)a + sizeof(unsigned long)));
    double d_b = *((const double*) ((const uint8_t *)b + sizeof(unsigned long)));

    if (d_a == d_b)
        return 0;
    else
        return d_a < d_b ? 1 : -1;
}

// TODO not working
void sort_docs(map<unsigned long, double> docs, const unsigned response_imgs, uint8_t** res) {
    const size_t single_res_len = sizeof(unsigned long) + sizeof(double);

    *res = (uint8_t*)malloc(docs.size() * single_res_len);
    int pos = 0;
    for (map<unsigned long, double>::iterator l = docs.begin(); l != docs.end() ; ++l) {
        memcpy(res + pos * single_res_len, &l->first, sizeof(unsigned long));
        memcpy(res + pos * single_res_len + sizeof(unsigned long), &l->second, sizeof(double));
        pos++;
    }

    qsort(res, docs.size(), single_res_len, compare_results);
    for (size_t m = 0; m < response_imgs; ++m) {
        unsigned long a;
        double b;

        memcpy(&a, res + m * single_res_len, sizeof(unsigned long));
        memcpy(&b, res + m * single_res_len + sizeof(unsigned long), sizeof(double));

        outside_util::printf("%lu %f\n", a, b);
    }
    outside_util::printf("\n");
}
