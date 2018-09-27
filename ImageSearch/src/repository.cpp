#include "repository.h"

#include <string.h>
#include "definitions.h"
#include "imgsrc_definitions.h"
#include "trusted_util.h"
#include "trusted_crypto.h"

repository* repository_init(unsigned nr_clusters, size_t desc_len, const char* train_technique) {
    repository* r = (repository*)malloc(sizeof(repository));

    r->desc_len = desc_len;
    r->cluster_count = nr_clusters;
    r->train_technique = (char*)malloc(strlen(train_technique) + 1);
    memcpy(r->train_technique, train_technique, strlen(train_technique) + 1);

    r->k = new BagOfWordsTrainer(nr_clusters, desc_len);

    // generate keys
    r->kf = (uint8_t*)malloc(SHA256_BLOCK_SIZE);
    r->ke = (uint8_t*)malloc(SYMM_KEY_SIZE);
    //tcrypto::random(r->kf, SHA256_BLOCK_SIZE);
    //tcrypto::random(r->ke, AES_KEY_SIZE);
    memset(r->kf, 0x00, SHA256_BLOCK_SIZE);
    memset(r->ke, 0x00, SYMM_KEY_SIZE);

    // init key map
    r->centre_keys = (uint8_t**)malloc(sizeof(uint8_t*) * nr_clusters);
    for (unsigned i = 0; i < nr_clusters; ++i) {
        uint8_t* key = (uint8_t*)malloc(SHA256_OUTPUT_SIZE);
        tcrypto::hmac_sha256(key, &i, sizeof(unsigned), r->kf, SHA256_BLOCK_SIZE);

        r->centre_keys[i] = key;
    }

    // init counters
    r->counters = (unsigned*)malloc(nr_clusters * sizeof(unsigned));
    memset(r->counters, 0x00, nr_clusters * sizeof(unsigned));
    r->total_docs = 0;

    // establish connection to uee
    r->server_socket = outside_util::open_uee_connection();

    // init resource pool for threads
    const unsigned nr_threads = trusted_util::thread_get_count();
    r->resource_pool = (thread_resource*)malloc(sizeof(thread_resource)*nr_threads);
    for (unsigned i = 0; i < nr_threads; ++i) {
        r->resource_pool[i].max_add_req_len = sizeof(unsigned char) + sizeof(size_t) + ADD_MAX_BATCH_LEN * PAIR_LEN;
        r->resource_pool[i].add_req_buffer = (uint8_t*)malloc(r->resource_pool[i].max_add_req_len);
        r->resource_pool[i].server_socket = outside_util::open_uee_connection(); // open thread's connection to server
    }

    return r;
}

benchmark* benchmark_init() {
    return (benchmark*)malloc(sizeof(benchmark));
}

void benchmark_clear(benchmark* b) {
    free(b);
}

void repository_clear(repository* r) {
    free(r->kf);
    free(r->ke);
    free(r->train_technique);

    for (size_t i = 0; i < r->k->nr_centres(); i++)
        free(r->centre_keys[i]);
    free(r->centre_keys);

    free(r->counters);

    // cleanup bag-of-words class
    r->k->cleanup();
    delete r->k;

    // clear uee and close connection
    //const unsigned char clear_op = OP_UEE_CLEAR;
    //size_t res_len;
    void* res;
    //outside_util::uee_process(r->server_socket, &res, &res_len, &clear_op, 1); // TODO send with socket_send
    //outside_util::outside_free(res);
    outside_util::close_uee_connection(r->server_socket);

    // clean resource pool
    const unsigned nr_threads = trusted_util::thread_get_count();
    for (unsigned i = 0; i < nr_threads; ++i) {
        free(r->resource_pool[i].add_req_buffer);
        outside_util::close_uee_connection(r->resource_pool[i].server_socket);
    }

    free(r->resource_pool);

    free(r);
}
