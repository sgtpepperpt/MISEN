#ifndef K_BAGOFWORDS_H
#define K_BAGOFWORDS_H

#include <stdlib.h>
#include <vector>

#include "kmeans.h"

typedef struct img_descriptor {
    unsigned count;
    float* descriptors;
} img_descriptor;

using namespace std;

class BagOfWordsTrainer {
public:
    BagOfWordsTrainer();//remove
    BagOfWordsTrainer(unsigned cluster_count, unsigned attempts, size_t desc_len);
    void add(img_descriptor* descriptors);

    void clear();

    // Returns trained vocabulary (i.e. cluster centres).
    img_descriptor* cluster();
    img_descriptor* cluster(float* descriptors, size_t nr_rows, size_t row_len);

private:
    unsigned cluster_count;
    unsigned attempts;
    vector<img_descriptor*> descriptors;
    size_t size;
    size_t desc_len;
};

#endif
