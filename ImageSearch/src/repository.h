#ifndef __REPOSITORY_H
#define __REPOSITORY_H

#include <stdlib.h>
#include <stdint.h>
#include "training.h"

typedef struct thread_resource {
    int server_socket;
    size_t max_add_req_len;
    uint8_t* add_req_buffer;
} thread_resource;

typedef struct repository {
    // repository constants
    size_t cluster_count;
    size_t desc_len;

    BagOfWordsTrainer* k; // TODO deprecate

    // keys for server encryption
    uint8_t* kf; // used to derive kw for each centre
    uint8_t* ke;
    uint8_t** centre_keys; // store kw for each centre

    // overall counters
    unsigned* counters;
    unsigned total_docs;

    // uee socket for main thread
    int server_socket;

    thread_resource* resource_pool;
} repository;

repository* repository_init(unsigned nr_clusters, size_t desc_len);
void repository_clear(repository* r);

#endif
