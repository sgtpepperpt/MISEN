#include "scoring.h"

#include <string.h>
#include <math.h>

#include "outside_util.h"

using namespace std;

double* calc_idf(size_t total_docs, unsigned* counters, const size_t nr_centres) {
    double* idf = (double*)malloc(nr_centres * sizeof(double));
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

void sort_docs(std::map<unsigned long, double> docs, const unsigned int response_imgs, uint8_t** res) {

}
