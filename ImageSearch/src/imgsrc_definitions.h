#ifndef __IMGSRC_DEFINITIONS_H
#define __IMGSRC_DEFINITIONS_H

#include "trusted_crypto.h"

#define LABEL_LEN (SHA256_OUTPUT_SIZE)
#define UNENC_VALUE_LEN (LABEL_LEN + sizeof(unsigned long) + sizeof(unsigned)) // (32 + 8 + 4 = 44)
#define ENC_VALUE_LEN (UNENC_VALUE_LEN + 4) // AES padding (44 + 4 = 48)

#define ADD_MAX_BATCH_LEN 2000
#define SEARCH_MAX_BATCH_LEN 1000

#define RESPONSE_DOCS 100

// derived values
const size_t pair_len = LABEL_LEN + ENC_VALUE_LEN;

#endif