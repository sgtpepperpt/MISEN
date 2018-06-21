#include "thread_pool.h"

void* enter_enclave_thread(void* args) {
    ecall_thread_enter();
    return NULL;
}

thread_pool* init_thread_pool(unsigned thread_count) {
    thread_pool* pool = (thread_pool*)malloc(sizeof(thread_pool));
    pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);
    pool->count = thread_count;

    for (unsigned i = 0; i < thread_count; ++i) {
        void* arg = NULL;
        pthread_create(&(pool->threads[i]), NULL, enter_enclave_thread, arg);
    }

    return pool;
}

void delete_thread_pool(thread_pool* pool) {
    for (unsigned i = 0; i < pool->count; ++i)
        pthread_join(pool->threads[i], 0);

    free(pool->threads);
    free(pool);
}
