#ifndef __IMG_PROCESSING_H
#define __IMG_PROCESSING_H

#include <stdlib.h>
#include <stdint.h>
#include "seq-kmeans/bagofwords.h"

unsigned* process_new_image(BagOfWordsTrainer* k, const uint8_t* in, const size_t in_len);

#endif
