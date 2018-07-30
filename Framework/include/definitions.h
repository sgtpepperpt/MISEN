#ifndef __DEFINITIONS_H
#define __DEFINITIONS_H

#define IEE_HOSTNAME "localhost"
#define IEE_PORT 7910

#define UEE_HOSTNAME "localhost"
#define UEE_PORT 7911

// op codes
#define OP_IEE_INIT 'i'
#define OP_IEE_TRAIN_ADD 'a'

#define OP_IEE_TRAIN_LSH '7'
#define OP_IEE_ADD_LSH '1'

#define OP_IEE_TRAIN 'k'
#define OP_IEE_ADD 'n'
#define OP_IEE_SEARCH 's'
#define OP_IEE_CLEAR 'c'
#define OP_IEE_LOAD 'l'
#define OP_IEE_WRITE_MAP 'w'
#define OP_IEE_READ_MAP 'r'
#define OP_IEE_SET_CODEBOOK 'x'

#define OP_UEE_INIT 'i'
#define OP_UEE_ADD 'a'
#define OP_UEE_SEARCH 's'
#define OP_UEE_WRITE_MAP 'w'
#define OP_UEE_READ_MAP 'r'
#define OP_UEE_CLEAR 'c'

#endif
