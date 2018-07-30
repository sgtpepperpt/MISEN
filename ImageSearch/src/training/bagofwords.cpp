#include "bagofwords.h"

#include <string.h>
#include "kmeans.h"

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

int BagOfWordsTrainer::is_full_after(size_t to_add) {
    outside_util::printf("total_descriptors %lu\n", total_descriptors);
    return total_descriptors + to_add >= MAX_DESCRIPTORS_MEM;
}

size_t BagOfWordsTrainer::nr_centres() {
    return kmeans->k;
}

size_t BagOfWordsTrainer::get_desc_len() {
    return kmeans->desc_len;
}

float* BagOfWordsTrainer::get_centre(int k) {
    return kmeans->centres + k * kmeans->desc_len;
}

void BagOfWordsTrainer::cleanup() {
    while(!descriptors.empty()) {
        free(descriptors.back()->descriptors);
        free(descriptors.back());
        descriptors.pop_back();
    }

    total_descriptors = 0;
    descriptors.clear();
}

const float* const BagOfWordsTrainer::get_all_centres() {
    return kmeans->centres;
}

void BagOfWordsTrainer::store(float* p) {
    centroids = p;
}

void BagOfWordsTrainer::cluster() {
    if(descriptors.empty())
        return;

    float* all_descriptors = (float*)malloc(total_descriptors * kmeans->desc_len * sizeof(float));
    //printf("total %lu desc %lu - %lu %p\n", total_descriptors, kmeans->desc_len, total_descriptors * kmeans->desc_len * sizeof(float), all_descriptors);
    float* tmp = all_descriptors;
    for (list<img_descriptor*>::iterator it = descriptors.begin(); it != descriptors.end(); ++it) {
        memcpy(tmp, (*it)->descriptors, kmeans->desc_len * (*it)->count * sizeof(float));
        tmp += kmeans->desc_len * (*it)->count;// * sizeof(float);
    }

    if(!kmeans->started)
        train_kmeans_init(all_descriptors, total_descriptors, kmeans);
    else
        train_kmeans(all_descriptors, total_descriptors, kmeans);

    free(all_descriptors);

    // this might be used to return the centres after clustering
    //float* res = (float*)malloc(kmeans->k * kmeans->desc_len * sizeof(float));
    //memcpy(res, kmeans->centres, kmeans->k * kmeans->desc_len * sizeof(float));

    // remove used descriptors
    cleanup();
}

void BagOfWordsTrainer::set_centres(void* centres) {// TODO used for debugging
    kmeans->centres = (float*)centres;
}
/*
void BagOfWordsTrainer::set_nr_centres(size_t nr_centres){
    kmeans->
}*/
