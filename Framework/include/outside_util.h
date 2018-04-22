#ifndef __OUTSIDE_UTIL_H_
#define __OUTSIDE_UTIL_H_

#include <stdlib.h>
#include "types.h"

#ifndef _INC_FCNTL
#define _INC_FCNTL

#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_APPEND    0x0008
#define O_ASYNC     0x0200

#endif

namespace outside_util {
    // file i/o
    int open(const char *filename, int mode);
    ssize_t read(int file, void *buf, size_t len);
    ssize_t write(int file, void *buf, size_t len);
    void close(int file);

    // uee communication
    int open_uee_connection();
    void uee_process(const int socket, void **out, size_t *out_len, const void *in, const size_t in_len);
    void close_uee_connection(const int socket);

    // outside allocation
    void* outside_malloc(size_t length);
    void outside_free(void *pointer);

    // misc
    void printf(const char *fmt, ...);
    void exit(int status);
    untrusted_time curr_time();

    // generic, implementable in extern
    int process(void **out, size_t *out_len, const void *in, const size_t in_len);
}
#endif
