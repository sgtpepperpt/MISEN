#ifndef __OCALL_WRAPPER_H_
#define __OCALL_WRAPPER_H_

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

#if defined(__cplusplus)
extern "C" {
#endif

int sgx_open(const char* filename, int mode);
ssize_t sgx_read(int file, void *buf, size_t len);
ssize_t sgx_write(int file, void *buf, size_t len);
void sgx_close(int file);

void sgx_printf(const char *fmt, ...);
void sgx_exit(int status);
untrusted_time sgx_curr_time();

int sgx_open_uee_connection();
void sgx_uee_process(const int socket, void** out, size_t* out_len, const void* in, const size_t in_len);
void sgx_close_uee_connection(const int socket);

void* sgx_untrusted_malloc(size_t length);
void sgx_untrusted_free(void*  pointer);

int sgx_ocall_process(void** out, size_t* out_len, const void* in, const size_t in_len);

#if defined(__cplusplus)
}
#endif

#endif
