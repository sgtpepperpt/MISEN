#ifndef __SCORING_H
#define __SCORING_H

#include <stdlib.h>
#include <stdint.h>
#include <map>

double* calc_idf(size_t total_docs, unsigned* counters, const size_t nr_centres);
void weight_idf(double *idf, unsigned* frequencies, const size_t nr_centres);

void sort_docs(std::map<unsigned long, double> docs, const unsigned int response_imgs, uint8_t** res);
int compare_results(const void *a, const void *b);
#endif
