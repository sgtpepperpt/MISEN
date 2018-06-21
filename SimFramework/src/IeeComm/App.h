#ifndef _APP_H_
#define _APP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

// sgx
#include "Enclave.h"

#include "definitions.h"
#include "untrusted_util.h"
#include "ocall.h"
#include "thread_pool.h"

#if defined(__cplusplus)
extern "C" {
#endif

//static void close_all(int signum);
//static void* process_client(void* args);

#if defined(__cplusplus)
}
#endif

#endif
