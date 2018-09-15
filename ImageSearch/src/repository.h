#ifndef __REPOSITORY_H
#define __REPOSITORY_H

#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include "training.h"

using namespace std;

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

    float** lsh;

    // uee socket for main thread
    int server_socket;

    thread_resource* resource_pool;
} repository;

typedef struct benchmark {
    size_t count_adds = 0;
    size_t count_searches = 0;
    double total_add_time = 0, total_add_time_server = 0;
    double total_search_time = 0, total_search_time_server = 0;
} benchmark;


benchmark* benchmark_init();
void benchmark_clear(benchmark* b);

repository* repository_init(unsigned nr_clusters, size_t desc_len);
void repository_clear(repository* r);

#endif
