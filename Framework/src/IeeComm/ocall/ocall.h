#ifndef _OCALL_H__
#define _OCALL_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include "Enclave_u.h"
#include "net_util.h"
#include "types.h"
#include <sys/time.h>
#include <time.h>


#if defined(__cplusplus)
extern "C" {
#endif

int ocall_open(const char* filename, int mode);
ssize_t ocall_read(int file, void *buf, size_t len);
ssize_t ocall_write(int file, void *buf, size_t len);
void ocall_close(int file);

void ocall_print_string(const char *str);
void ocall_exit(int code);
untrusted_time ocall_curr_time();

int ocall_open_uee_connection();
void ocall_close_uee_connection(const int socket);

void* ocall_untrusted_malloc(size_t length);
void ocall_untrusted_free(void** pointer);

int ocall_process(void** out, size_t* out_len, const void* in, const size_t in_len);

#if defined(__cplusplus)
}
#endif

#endif
