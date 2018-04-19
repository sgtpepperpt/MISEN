#ifndef BAGOFWORDS_H
#define BAGOFWORDS_H

#include <stdlib.h>
#include <list>
#include <vector>

#include "../../../Framework/include/untrusted_util.h"

#include "kmeans.h"

#define MAX_DESCRIPTORS_MEM 80000

typedef struct img_descriptor {
    unsigned count;
    float* buffer;
} img_descriptor;

class BagOfWordsTrainer {
public:
    BagOfWordsTrainer(size_t cluster_count, size_t desc_len);
    ~BagOfWordsTrainer();

    void add_descriptors(img_descriptor *descriptors);

    void cleanup();
    int is_full();
    size_t nr_centres();
    size_t desc_len();
    float* get_centre(int k);

    img_descriptor* cluster();
    //vector<unsigned> closest(float* descriptors, size_t nr_descriptors);

    int* frequencies;
private:
    std::list<img_descriptor*> descriptors;
    size_t total_descriptors;

    kmeans_data* kmeans;
};

#endif
