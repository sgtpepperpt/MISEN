#include "bagofwords.h"

using namespace std;

BagOfWordsTrainer::BagOfWordsTrainer() {

}

BagOfWordsTrainer::BagOfWordsTrainer(unsigned _cluster_count, unsigned _attempts, size_t _desc_len) {
    cluster_count = _cluster_count;
    attempts = _attempts;
    desc_len = _desc_len;
    size = 0;
    previous_centres = NULL;
}

void BagOfWordsTrainer::add(img_descriptor* _descriptors) {
    //assert(!_descriptors.empty());
    //assert(desc_len == _descriptors->each_desc_len);
    size += _descriptors->count;
    descriptors.push_back(_descriptors);
}

int BagOfWordsTrainer::is_full() {
    return descriptors.size() >= 10; //TODO fix clear
}

int BagOfWordsTrainer::nr_images() {
    return descriptors.size();
}

size_t BagOfWordsTrainer::get_desc_len() {
    return desc_len;
}

void BagOfWordsTrainer::clear() {
    for(size_t i = 0; i < descriptors.size(); i++) {
        free(descriptors[i]->descriptors);
        free(descriptors[i]);
    }

    descriptors.clear();
    size = 0;
}

img_descriptor* BagOfWordsTrainer::cluster() {
    assertion(!descriptors.empty());

    unsigned pos = 0;
    float *mergedDescriptors;
    if(previous_centres) {
        sgx_printf("previous to be included\n");
        mergedDescriptors = (float *) malloc((previous_centres->count + size) * desc_len * sizeof(float));
        for (size_t i = 0; i < previous_centres->count; i++) {
            for (unsigned k = 0; k < desc_len; ++k) {
                mergedDescriptors[pos++] = previous_centres->descriptors[i * desc_len + k];
            }
        }
    }
    else {
        mergedDescriptors = (float*)malloc(size * desc_len * sizeof(float));
    }

    for (size_t i = 0; i < descriptors.size(); i++) {
        for (unsigned j = 0; j < descriptors[i]->count; ++j) {
            for (unsigned k = 0; k < desc_len; ++k) {
                mergedDescriptors[pos++] = descriptors[i]->descriptors[j * desc_len + k];
                //printf("%f ", mergedDescriptors[pos]);
            }
        }
    }

    return cluster(mergedDescriptors, size, desc_len);
}

img_descriptor* BagOfWordsTrainer::cluster(float* all_descriptors, size_t nr_rows, size_t desc_len) {
    img_descriptor* centres = (img_descriptor*)malloc(sizeof(img_descriptor));
    centres->count = cluster_count;
    centres->descriptors = (float*)malloc(cluster_count * desc_len * sizeof(float));

    int *labels = (int*)malloc(nr_rows * sizeof(int));

    if(previous_centres)
        seqkmeans(all_descriptors, nr_rows, desc_len, cluster_count, labels, attempts, centres->descriptors, 2, 10);
    else
        seqkmeans(all_descriptors, nr_rows, desc_len, cluster_count, labels, attempts, centres->descriptors, 0, 1);

    free(labels);
    free(all_descriptors);

    if(previous_centres) {
        free(previous_centres->descriptors);
        free(previous_centres);
    }

    previous_centres = centres;

    return centres;
}
