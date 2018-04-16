#ifndef SGX_IMAGE_SEARCH_TRAINING_H
#define SGX_IMAGE_SEARCH_TRAINING_H

#include <stdlib.h>
#include <stdint.h>

#include "seq-kmeans/bagofwords.h"
#include "util.h"

void train_add_image(BagOfWordsTrainer* k, void** out, size_t* out_len, const uint8_t* in, const size_t in_len);
void train_kmeans(BagOfWordsTrainer* k, void** out, size_t* out_len, const uint8_t* in, const size_t in_len);

#endif
