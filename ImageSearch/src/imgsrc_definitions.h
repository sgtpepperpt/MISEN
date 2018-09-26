#ifndef __IMGSRC_DEFINITIONS_H
#define __IMGSRC_DEFINITIONS_H

#include "trusted_crypto.h"

#define LABEL_LEN (SHA256_OUTPUT_SIZE)
#define UNENC_VALUE_LEN (LABEL_LEN + sizeof(unsigned long) + sizeof(unsigned)) // (32 + 8 + 4 = 44)
#define ENC_VALUE_LEN (UNENC_VALUE_LEN) //(SYMM_ENC_SIZE(UNENC_VALUE_LEN)) // AES padding (44 + 4 = 48) in hw, 44 + sodium exp (24) in sw

#define ADD_MAX_BATCH_LEN 2000
#define SEARCH_MAX_BATCH_LEN 1000

#define RESPONSE_DOCS 15

// derived values
#define PAIR_LEN (LABEL_LEN + ENC_VALUE_LEN)

#endif
