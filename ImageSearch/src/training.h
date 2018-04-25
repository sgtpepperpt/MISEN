#ifndef __TRAINING_H
#define __TRAINING_H

#include <stdlib.h>
#include <stdint.h>

#include "seq-kmeans/bagofwords.h"
#include "util.h"

void train_add_image(BagOfWordsTrainer* k, unsigned long id, size_t nr_desc, const uint8_t* in, const size_t in_len);
void train_kmeans(BagOfWordsTrainer* k);

#endif
