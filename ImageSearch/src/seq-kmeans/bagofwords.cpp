#include "bagofwords.h"

using namespace std;

BagOfWordsTrainer::BagOfWordsTrainer(size_t cluster_count, size_t desc_len) {
    total_descriptors = 0;

    frequencies = (int*)malloc(cluster_count * sizeof(int));
    memset(frequencies, 0x00, cluster_count * sizeof(int));

    kmeans = (kmeans_data*)malloc(sizeof(kmeans_data));
    kmeans->desc_len = desc_len;
    kmeans->k = cluster_count;
    kmeans->centres = (float*)malloc(kmeans->k * desc_len * sizeof(float));
    kmeans->counters = (unsigned*)malloc(kmeans->k * sizeof(unsigned));
    kmeans->started = 0;
}

BagOfWordsTrainer::~BagOfWordsTrainer() {
    free(frequencies);

    free(kmeans->centres);
    free(kmeans->counters);
    free(kmeans);
}

void BagOfWordsTrainer::add_descriptors(img_descriptor* _descriptors) {
    total_descriptors += _descriptors->count;
    descriptors.push_back(_descriptors);
}

int BagOfWordsTrainer::is_full() {
    sgx_printf("total_descriptors %lu\n", total_descriptors);
    //return descriptors.size() >= 10;
    return total_descriptors >= MAX_DESCRIPTORS_MEM;
}

size_t BagOfWordsTrainer::nr_centres() {
    return kmeans->k;
}

size_t BagOfWordsTrainer::desc_len() {
    return kmeans->desc_len;
}

float* BagOfWordsTrainer::get_centre(int k) {
    return kmeans->centres + k * kmeans->desc_len;
}

void BagOfWordsTrainer::cleanup() {
    while(!descriptors.empty()) {
        free(descriptors.back()->buffer);
        free(descriptors.back());
        descriptors.pop_back();
    }

    total_descriptors = 0;
    descriptors.clear();
}

img_descriptor* BagOfWordsTrainer::cluster() {
    if(descriptors.empty()) {
        img_descriptor* res = (img_descriptor*)malloc(sizeof(img_descriptor)); //TODO remove later if k is known
        res->count = kmeans->k;
        res->buffer = kmeans->centres;

        return res;
    }

    float *all_descriptors = (float*)malloc(total_descriptors * kmeans->desc_len * sizeof(float));
    //sgx_printf("total %lu desc %lu - %lu %p\n", total_descriptors, kmeans->desc_len, total_descriptors * kmeans->desc_len * sizeof(float), all_descriptors);
    float *tmp = all_descriptors;
    for (list<img_descriptor*>::iterator it = descriptors.begin(); it != descriptors.end(); ++it) {
        memcpy(tmp, (*it)->buffer, kmeans->desc_len * (*it)->count * sizeof(float));
        tmp += kmeans->desc_len * (*it)->count;// * sizeof(float);
    }

    if(!kmeans->started)
        online_kmeans_init(all_descriptors, total_descriptors, kmeans);
    else
        online_kmeans(all_descriptors, total_descriptors, kmeans);

    free(all_descriptors);

    img_descriptor* res = (img_descriptor*)malloc(sizeof(img_descriptor)); //TODO remove later if k is known
    res->count = kmeans->k;
    res->buffer = (float*)malloc(kmeans->k * kmeans->desc_len * sizeof(float));
    memcpy(res->buffer, kmeans->centres, kmeans->k * kmeans->desc_len * sizeof(float));

    // remove used descriptors
    cleanup();

    return res;
}
