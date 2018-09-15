#ifndef __DEFINITIONS_H
#define __DEFINITIONS_H

#define IEE_HOSTNAME "localhost"
#define IEE_PORT 7910

#define UEE_HOSTNAME "localhost"
#define UEE_PORT 7911

// op codes
#define OP_IEE_INIT 'a'
#define OP_IEE_TRAIN_ADD 'b'

#define OP_IEE_TRAIN_LSH 'c'
#define OP_IEE_ADD_LSH 'd'

#define OP_IEE_TRAIN 'e'
#define OP_IEE_ADD 'f'
#define OP_IEE_SEARCH 'g'
#define OP_IEE_CLEAR 'h'
#define OP_IEE_LOAD 'i'
#define OP_IEE_WRITE_MAP 'j'
#define OP_IEE_READ_MAP 'k'
#define OP_IEE_SET_CODEBOOK_CLIENT_KMEANS 'l'
#define OP_IEE_SET_CODEBOOK_CLIENT_LSH 'm'

#define OP_UEE_INIT 'n'
#define OP_UEE_ADD 'o'
#define OP_UEE_SEARCH 'p'
#define OP_UEE_WRITE_MAP 'q'
#define OP_UEE_READ_MAP 'r'
#define OP_UEE_CLEAR 's'

#define OP_RBISEN '1'
#define OP_MISEN_QUERY '2'

// TODO as arguments, not preprocessor
#define C_KMEANS 1
#define C_LSH 2
#define CLUSTERING C_KMEANS

#define OP_IEE_DUMP_BENCH 'v'
#define OP_UEE_DUMP_BENCH 'x'

#define PRINT_DEBUG 0

#endif
