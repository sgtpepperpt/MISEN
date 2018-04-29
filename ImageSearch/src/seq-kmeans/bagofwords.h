#ifndef BAGOFWORDS_H
#define BAGOFWORDS_H

#include <stdlib.h>
#include <list>
#include <vector>

#include "outside_util.h"
#include "kmeans.h"

#define MAX_DESCRIPTORS_MEM 180000

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
    int is_full_after(size_t to_add);
    size_t nr_centres();
    size_t desc_len();
    float* get_centre(int k);
    const float* const get_all_centres();

    void cluster();
    //vector<unsigned> closest(float* descriptors, size_t nr_descriptors);

    int* frequencies;

    void set_centres(void* centres);

private:
    std::list<img_descriptor*> descriptors;
    size_t total_descriptors;

    kmeans_data* kmeans;
};

#endif
