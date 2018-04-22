#include "Enclave.h"

#include <stdint.h>
#include <string.h>
#include <mutex>
#include <condition_variable>

#include "outside_util.h"
#include "trusted_crypto.h"
#include "thread_handler.h"
#include "extern_lib.h"

void ecall_init_enclave(unsigned nr_threads) {
    thread_handler_init(nr_threads);

    // let extern lib do its own init
    extern_lib::init();
}

void ecall_thread_enter() {
    // register thread as entering
    thread_data* my_data = thread_handler_add();

    while (1) {
        // wait until we have work
        std::unique_lock<std::mutex> lock(*my_data->lock);
        my_data->cond_var->wait(lock, [&my_data]{return my_data->ready;});

        // extern lib's own handling
        void* res = my_data->task(my_data->task_args); // TODO pass res to thread_handler via my_data?

        my_data->ready = 0;
        my_data->done = 1;
        lock.unlock();
        my_data->cond_var->notify_one();
    }
}

void ecall_process(void** out, size_t* out_len, const void* in, const size_t in_len) {
    /*// TODO do not use these hardcoded values
    uint8_t key[AES_BLOCK_SIZE];
    memset(key, 0x00, AES_BLOCK_SIZE);

    uint8_t ctr[AES_BLOCK_SIZE];
    memset(ctr, 0x00, AES_BLOCK_SIZE);

    // decrypt input
    uint8_t* in_unenc = (uint8_t*)malloc(in_len);
    c_decrypt(in_unenc, (uint8_t *)in, in_len, key, ctr);

    // prepare unencrypted output
    uint8_t* out_unenc = NULL;
    process_message(&out_unenc, out_len, in_unenc, in_len);

    // encrypt output
    memset(ctr, 0x00, AES_BLOCK_SIZE);
    *out = outside_malloc(*out_len);
    c_encrypt(*out, out_unenc, *out_len, key, ctr);

    // cleanup
    free(in_unenc);
    free(out_unenc);*/

    extern_lib::process_message((uint8_t **) out, out_len, (const uint8_t *) in, in_len);

    // step needed to copy from unencrypted inside to encrypted outside
    uint8_t* res = (uint8_t*) outside_util::outside_malloc(*out_len);
    memcpy(res, *out, *out_len);

    free(*out);

    *out = res;
}
