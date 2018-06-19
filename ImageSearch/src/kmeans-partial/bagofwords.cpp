#include "bagofwords.h"
#include <vector>
using namespace std;

BagOfWordsTrainer::BagOfWordsTrainer() {

}

BagOfWordsTrainer::BagOfWordsTrainer(unsigned _cluster_count, size_t _desc_len) {
    desc_len = _desc_len;
    cluster_count = _cluster_count;
}

size_t BagOfWordsTrainer::get_desc_len() {
    return desc_len;
}

size_t BagOfWordsTrainer::nr_centres() {
    return cluster_count;
}

void BagOfWordsTrainer::store(float* p) {
    centroids = p;
}

float* BagOfWordsTrainer::get_centre(int k) {
    return centroids + k * desc_len;
}
