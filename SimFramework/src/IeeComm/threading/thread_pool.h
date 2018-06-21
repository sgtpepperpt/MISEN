#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H

#include <stdlib.h>
#include <stdio.h>
#include <queue>
#include <unistd.h>
#include <pthread.h>

#include "Enclave.h"

#include "ocall.h"
#include "thread_pool.h"

typedef struct thread_pool {
    pthread_t *threads;
    unsigned count;
} thread_pool;

thread_pool* init_thread_pool(unsigned thread_count);
void delete_thread_pool(thread_pool* pool);

#endif
