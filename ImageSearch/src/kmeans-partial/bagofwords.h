#ifndef BAGOFWORDS_H
#define BAGOFWORDS_H

#include <stdlib.h>
#include <vector>

#include "kmeans.h"


using namespace std;

class BagOfWordsTrainer {
public:
    BagOfWordsTrainer();//remove
    BagOfWordsTrainer(unsigned cluster_count, size_t desc_len);

    size_t get_desc_len();
    size_t nr_centres();

    void store(float* p);
    float* get_centre(int k);
private:
    size_t desc_len;
    size_t cluster_count;
    float* centroids;
};

#endif
