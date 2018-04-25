#ifndef __IMG_PROCESSING_H
#define __IMG_PROCESSING_H

#include <stdlib.h>
#include <stdint.h>
#include "seq-kmeans/bagofwords.h"

unsigned* process_new_image(BagOfWordsTrainer* k, const size_t nr_desc, float* descriptors);

#endif
