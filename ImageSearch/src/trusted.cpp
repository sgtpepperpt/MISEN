#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <float.h>
#include <map>

// includes from framework
#include "types.h"
#include "definitions.h"
#include "trusted_util.h"
#include "outside_util.h"
#include "trusted_crypto.h"
#include "extern_lib.h" // defines the functions we implement here

// training and kmeans
#include "scoring.h"
#include "training.h"
#include "util.h"
#include "seq-kmeans/util.h"

#define LABEL_LEN (SHA256_OUTPUT_SIZE)
#define UNENC_VALUE_LEN (LABEL_LEN + sizeof(unsigned long) + sizeof(unsigned)) // (32 + 8 + 4 = 44)
#define ENC_VALUE_LEN (UNENC_VALUE_LEN + 4) // AES padding (44 + 4 = 48)

#define ADD_MAX_BATCH_LEN 2000
#define SEARCH_MAX_BATCH_LEN 1000

#define RESPONSE_DOCS 100

// derived values
const size_t pair_len = LABEL_LEN + ENC_VALUE_LEN;

typedef struct thread_resource {
    int server_socket;
    size_t max_add_req_len;
    uint8_t* add_req_buffer;
} thread_resource;

using namespace std;
using namespace outside_util;

BagOfWordsTrainer* k;
int server_socket;

// keys for server encryption
void* kf;
void* ke;

// store kw for each centre
uint8_t** centre_keys;

// overall counters
unsigned* counters;
unsigned total_docs;

thread_resource* resource_pool;

void repository_init(unsigned nr_clusters, size_t desc_len) {
    k = new BagOfWordsTrainer(nr_clusters, desc_len);

    // generate keys
    kf = malloc(SHA256_BLOCK_SIZE);
    ke = malloc(AES_KEY_SIZE);
    tcrypto::random(kf, SHA256_BLOCK_SIZE);
    tcrypto::random(ke, AES_KEY_SIZE);

    // init key map
    centre_keys = (uint8_t**)malloc(sizeof(uint8_t*) * nr_clusters);
    for (unsigned i = 0; i < nr_clusters; ++i) {
        uint8_t* key = (uint8_t*)malloc(SHA256_OUTPUT_SIZE);
        tcrypto::hmac_sha256(key, &i, sizeof(unsigned), kf, SHA256_BLOCK_SIZE);

        centre_keys[i] = key;
    }

    // init counters
    counters = (unsigned*)malloc(nr_clusters * sizeof(unsigned));
    memset(counters, 0x00, nr_clusters * sizeof(unsigned));
    total_docs = 0;

    // establish connection to uee
    server_socket = outside_util::open_uee_connection();

    // init resource pool for threads
    const unsigned nr_threads = trusted_util::thread_get_count();
    resource_pool = (thread_resource*)malloc(sizeof(thread_resource)*nr_threads);
    for (unsigned i = 0; i < nr_threads; ++i) {
        resource_pool[i].max_add_req_len = sizeof(unsigned char) + sizeof(size_t) + ADD_MAX_BATCH_LEN * pair_len;
        resource_pool[i].add_req_buffer = (uint8_t*)malloc(resource_pool[i].max_add_req_len);
        resource_pool[i].server_socket = outside_util::open_uee_connection(); // open thread's connection to server
    }
}

void repository_clear() {
    free(kf);
    free(ke);

    for (size_t i = 0; i < k->nr_centres(); i++)
        free(centre_keys[i]);
    free(centre_keys);

    free(counters);

    // cleanup bag-of-words class
    k->cleanup();
    delete k;

    outside_util::close_uee_connection(server_socket);

    // clean resource pool
    const unsigned nr_threads = trusted_util::thread_get_count();
    for (int i = 0; i < nr_threads; ++i) {
        free(resource_pool[i].add_req_buffer);
        outside_util::close_uee_connection(resource_pool[i].server_socket);
    }

    free(resource_pool);
}

#define PARALLEL_IMG_PROCESSING 1
typedef struct process_args {
    size_t start;
    size_t end;
    float* descriptors;
    unsigned* frequencies;
} process_args;

#if PARALLEL_IMG_PROCESSING
void* parallel_process(void* args) {
    process_args* arg = (process_args*)args;

    for (size_t i = arg->start; i < arg->end; ++i) {
        double min_dist = DBL_MAX;
        int pos = -1;

        for (size_t j = 0; j < k->nr_centres(); ++j) {
            double dist_to_centre = normL2Sqr(k->get_centre(j), arg->descriptors + i * k->desc_len(), k->desc_len());

            if(dist_to_centre < min_dist) {
                min_dist = dist_to_centre;
                pos = j;
            }
        }

        arg->frequencies[pos]++;
    }

    return NULL;
}
#endif

static unsigned* process_new_image(const uint8_t* in, const size_t in_len) {
    untrusted_time start = outside_util::curr_time();

    size_t nr_desc;
    memcpy(&nr_desc, in, sizeof(size_t));

    float* descriptors = (float*)(in + sizeof(size_t));
    outside_util::printf("proc: nr_descs %d\n", nr_desc);

    unsigned* frequencies = (unsigned*)malloc(k->nr_centres() * sizeof(unsigned));
    memset(frequencies, 0x00, k->nr_centres() * sizeof(unsigned));

#if PARALLEL_IMG_PROCESSING
    // initialisation
    unsigned nr_threads = trusted_util::thread_get_count();
    size_t desc_per_thread = nr_desc / nr_threads;

    process_args* args = (process_args*)malloc(nr_threads * sizeof(process_args));

    for (unsigned j = 0; j < nr_threads; ++j) {
        // each thread receives the generic pointers and the thread ranges
        args[j].descriptors = descriptors;
        args[j].frequencies = frequencies;

        if(j == 0) {
            args[j].start = 0;
            args[j].end = j * desc_per_thread + desc_per_thread;
        } else {
            args[j].start = j * desc_per_thread + 1;

            if (j + 1 == nr_threads)
                args[j].end = nr_desc;
            else
                args[j].end = j * desc_per_thread + desc_per_thread;
        }

        //outside_util::printf("start %lu end %lu\n", args[j].start, args[j].end);
        trusted_util::thread_add_work(parallel_process, args + j);
    }

    trusted_util::thread_do_work();
    free(args);
#else
    // for each descriptor, calculate its closest centre
    for (size_t i = 0; i < nr_desc; ++i) {
        double min_dist = DBL_MAX;
        int pos = -1;

        for (size_t j = 0; j < k->nr_centres(); ++j) {
            double dist_to_centre = normL2Sqr(k->get_centre(j), descriptors + i * k->desc_len(), k->desc_len());

            if(dist_to_centre < min_dist) {
                min_dist = dist_to_centre;
                pos = j;
            }
        }

        frequencies[pos]++;
    }
#endif

    untrusted_time end = outside_util::curr_time();
    outside_util::printf("elapsed %ld\n", trusted_util::time_elapsed_ms(start, end));

    return frequencies;
}

#define PARALLEL_ADD_IMG 1

#if PARALLEL_ADD_IMG
typedef struct img_add_args {
    unsigned tid;
    unsigned long id;
    unsigned* frequencies;
    size_t centre_pos_start;
    size_t centre_pos_end;
} img_add_args;

void* add_img_parallel(void* args) {
    img_add_args* arg = (img_add_args*)args;

    // used for aes TODO better
    unsigned char ctr[AES_BLOCK_SIZE];
    memset(ctr, 0x00, AES_BLOCK_SIZE);

    // prepare request to uee
    resource_pool[arg->tid].add_req_buffer[0] = OP_UEE_ADD;

    size_t to_send = arg->centre_pos_end - arg->centre_pos_start + 1;
    size_t centre_pos = arg->centre_pos_start;
    while (to_send > 0) {
        size_t batch_len = min(ADD_MAX_BATCH_LEN, to_send);

        // add number of pairs to buffer
        uint8_t* tmp = resource_pool[arg->tid].add_req_buffer + sizeof(unsigned char);
        memcpy(tmp, &batch_len, sizeof(size_t));
        tmp += sizeof(size_t);

        // add all batch pairs to buffer
        for (size_t i = 0; i < batch_len; ++i) {
            memset(ctr, 0x00, AES_BLOCK_SIZE);

            // calculate label based on current counter for the centre
            unsigned counter = counters[centre_pos];
            //outside_util::printf("(%lu/%lu/%lu) counter %u\n", arg->centre_pos_start, centre_pos, arg->centre_pos_end, counter);
            //outside_util::printf("%p %u\n", centre_keys[centre_pos], SHA256_OUTPUT_SIZE);
            tcrypto::hmac_sha256(tmp, &counter, sizeof(unsigned), centre_keys[centre_pos], SHA256_OUTPUT_SIZE);
            uint8_t* label = tmp; // keep a reference to the label
            tmp += LABEL_LEN;

            // increase the centre's counter, if present
            if (arg->frequencies[centre_pos])//TODO remove this if, counter is increased even if 0
                ++counters[centre_pos];

            // calculate value
            // label + img_id + frequency
            unsigned char value[UNENC_VALUE_LEN];
            memcpy(value, label, LABEL_LEN);
            memcpy(value + LABEL_LEN, &arg->id, sizeof(unsigned long));
            memcpy(value + LABEL_LEN + sizeof(unsigned long), &arg->frequencies[centre_pos], sizeof(unsigned));

            // encrypt value
            tcrypto::encrypt(tmp, value, UNENC_VALUE_LEN, ke, ctr);
            tmp += ENC_VALUE_LEN;

            to_send--;
            centre_pos++;
        }

        // send batch to server
        size_t res_len;
        void* res;
        outside_util::uee_process(resource_pool[arg->tid].server_socket, &res, &res_len, resource_pool[arg->tid].add_req_buffer, sizeof(unsigned char) + sizeof(size_t) + batch_len * pair_len);
        outside_util::outside_free(res); // discard ok response
    }

    return NULL;
}
#endif

static void add_image(uint8_t** out, size_t* out_len, const uint8_t* in, const size_t in_len) {
    // used for aes TODO better
    unsigned char ctr[AES_BLOCK_SIZE];
    memset(ctr, 0x00, AES_BLOCK_SIZE);

    // get image id from buffer
    unsigned long id;
    memcpy(&id, in, sizeof(unsigned long));
    outside_util::printf("add, id %lu\n", id);

    unsigned* frequencies = process_new_image(in + sizeof(unsigned long), in_len - sizeof(unsigned long));

    outside_util::printf("frequencies: ");
    for (size_t i = 0; i < k->nr_centres(); i++)
        outside_util::printf("%u ", frequencies[i]);
    outside_util::printf("\n");

#if PARALLEL_ADD_IMG
    const unsigned nr_threads = trusted_util::thread_get_count();
    size_t k_per_thread = k->nr_centres() / nr_threads;

    img_add_args* args = (img_add_args*)malloc(nr_threads * sizeof(img_add_args));
    for (unsigned j = 0; j < nr_threads; ++j) {
        // each thread receives the generic pointers and the thread ranges
        args[j].id = id;
        args[j].frequencies = frequencies;
        args[j].tid = j;

        if(j == 0) {
            args[j].centre_pos_start = 0;
            args[j].centre_pos_end = k_per_thread;
        } else {
            args[j].centre_pos_start = j * k_per_thread + 1;

            if (j + 1 == nr_threads)
                args[j].centre_pos_end = k->nr_centres() - 1;// TODO -1 because still not [a,b[
            else
                args[j].centre_pos_end = j * k_per_thread + k_per_thread;
        }

        outside_util::printf("start %lu end %lu\n", args[j].centre_pos_start, args[j].centre_pos_end);

        //outside_util::printf("start %lu end %lu\n", args[j].start, args[j].end);
        trusted_util::thread_add_work(add_img_parallel, args + j);
    }

    trusted_util::thread_do_work();

    // cleanup
    free(args);
#else
    // prepare request to uee
    const size_t pair_len = LABEL_LEN + ENC_VALUE_LEN;

    size_t max_req_len = sizeof(unsigned char) + sizeof(size_t) + ADD_MAX_BATCH_LEN * pair_len;
    uint8_t* req_buffer = (uint8_t*)malloc(max_req_len);
    req_buffer[0] = OP_UEE_ADD;

    size_t to_send = k->nr_centres();
    size_t centre_pos = 0;
    while (to_send > 0) {
        size_t batch_len = min(ADD_MAX_BATCH_LEN, to_send);

        // add number of pairs to buffer
        uint8_t* tmp = req_buffer + sizeof(unsigned char);
        memcpy(tmp, &batch_len, sizeof(size_t));
        tmp += sizeof(size_t);

        // add all batch pairs to buffer
        for (size_t i = 0; i < batch_len; ++i) {
            memset(ctr, 0x00, AES_BLOCK_SIZE);

            // calculate label based on current counter for the centre
            int counter = counters[centre_pos];
            tcrypto::hmac_sha256(tmp, &counter, sizeof(unsigned), centre_keys[centre_pos], LABEL_LEN);
            uint8_t* label = tmp; // keep a reference to the label
            tmp += LABEL_LEN;

            // increase the centre's counter, if present
            if(frequencies[centre_pos])//TODO remove this if, counter is increased even if 0
                ++counters[centre_pos];

            // calculate value
            // label + img_id + frequency
            unsigned char value[UNENC_VALUE_LEN];
            memcpy(value, label, LABEL_LEN);
            memcpy(value + LABEL_LEN, &id, sizeof(unsigned long));
            memcpy(value + LABEL_LEN + sizeof(unsigned long), &frequencies[centre_pos], sizeof(unsigned));

            // encrypt value
            tcrypto::encrypt(tmp, value, UNENC_VALUE_LEN, ke, ctr);
            tmp += ENC_VALUE_LEN;

            to_send--;
            centre_pos++;
        }

        // send batch to server
        size_t res_len;
        void* res;
        outside_util::uee_process(server_socket, &res, &res_len, req_buffer, sizeof(unsigned char) + sizeof(size_t) + batch_len * pair_len);
        outside_util::outside_free(res); // discard ok response
    }

    free(req_buffer);
#endif

    total_docs++;
    free(frequencies);

}

void search_image(uint8_t** out, size_t* out_len, const uint8_t* in, const size_t in_len) {
    using namespace outside_util;
    unsigned* frequencies = process_new_image(in, in_len);
    for (size_t i = 0; i < k->nr_centres(); ++i) {
        printf("%u ", frequencies[i]);
    }
    printf("\n");

    double* idf = calc_idf(total_docs, counters, k->nr_centres());
    for (size_t i = 0; i < k->nr_centres(); ++i) {
        printf("%lf ", idf[i]);
    }
    printf("\n");

    //weight_idf(idf, frequencies);
    for (size_t i = 0; i < k->nr_centres(); ++i) {
        printf("%lf ", idf[i]);
    }
    printf("\n");


    // calc size of request; ie sum of counters (for all centres of searched image)
    size_t nr_labels = 0;
    for (size_t i = 0; i < k->nr_centres(); ++i) {
        if(frequencies[i])
            nr_labels += counters[i];
    }

    // will hold result
    std::map<unsigned long, double> docs;

    // prepare request to uee
    size_t max_req_len = sizeof(unsigned char) + sizeof(size_t) + SEARCH_MAX_BATCH_LEN * LABEL_LEN;
    uint8_t* req_buffer = (uint8_t*)malloc(max_req_len);
    req_buffer[0] = OP_UEE_SEARCH;

    size_t to_send = nr_labels;
    size_t centre_pos = 0;
    size_t counter_pos = 0;

    unsigned batch_counter = 0;
    while (to_send > 0) {
        printf("(%02u) centre %lu; counter %lu \n", batch_counter++, centre_pos, counter_pos);

        size_t batch_len = min(SEARCH_MAX_BATCH_LEN, to_send);

        // add number of labels to buffer
        uint8_t* tmp = req_buffer + sizeof(unsigned char);
        memcpy(tmp, &batch_len, sizeof(size_t));
        tmp += sizeof(size_t);

        // add all batch labels to buffer
        for (size_t i = 0; i < batch_len; ++i) {
            // calc label
            tcrypto::hmac_sha256(tmp, &counter_pos, sizeof(unsigned), centre_keys[centre_pos], LABEL_LEN);
            tmp += LABEL_LEN;

            // update pointers
            ++counter_pos;
            if(counter_pos == counters[centre_pos]) {
                // search for the next centre where the searched image's frequency is non-zero
                while (!frequencies[++centre_pos])
                    ;
                counter_pos = 0;
            }
        }

        to_send -= batch_len;

        // perform request to uee
        size_t res_len;
        uint8_t* res;
        uee_process(server_socket, (void **)&res, &res_len, req_buffer, sizeof(unsigned char) + sizeof(size_t) + batch_len * LABEL_LEN);
        uint8_t* res_tmp = res;

        // process all answers
        for (size_t i = 0; i < batch_len; ++i) {
            // decode res
            uint8_t res_unenc[UNENC_VALUE_LEN];
            unsigned char ctr[AES_BLOCK_SIZE];
            memset(ctr, 0x00, AES_BLOCK_SIZE);

            tcrypto::decrypt(res_unenc, res_tmp, ENC_VALUE_LEN, ke, ctr);
            res_tmp += ENC_VALUE_LEN;

            int verify = memcmp(res_unenc, (req_buffer + sizeof(unsigned char) + sizeof(size_t)) + i * LABEL_LEN, LABEL_LEN);
            if(verify) {
                printf("Label verification doesn't match! Exit\n");
                exit(-1);
            }

            unsigned long id;
            memcpy(&id, res_unenc + LABEL_LEN, sizeof(unsigned long));

            unsigned frequency;
            memcpy(&frequency, res_unenc + LABEL_LEN + sizeof(unsigned long), sizeof(unsigned));

            if (docs[id])
                docs[id] += frequency? frequencies[i] * (1 + log10(frequency)) * idf[i] : 0;
            else
                docs[id] = frequency? frequencies[i] * (1 + log10(frequency)) * idf[i] : 0;
        }

        outside_util::outside_free(res);
    }

    free(idf);
    free(frequencies);

    // TODO randomise, do not ignore zero counters, fix batch search for 2000

    // calculate score and respond to client
    const unsigned response_imgs = min(docs.size(), RESPONSE_DOCS);
    const size_t single_res_len = sizeof(unsigned long) + sizeof(double);

    // put result in simple structure, to sort
    uint8_t* res;

    ////////////////////
    //sort_docs(docs, response_imgs, &res); // TODO
    res = (uint8_t*)malloc(docs.size() * single_res_len);
    int pos = 0;
    for (map<unsigned long, double>::iterator l = docs.begin(); l != docs.end() ; ++l) {
        memcpy(res + pos * single_res_len, &l->first, sizeof(unsigned long));
        memcpy(res + pos * single_res_len + sizeof(unsigned long), &l->second, sizeof(double));
        pos++;
    }

    qsort(res, docs.size(), single_res_len, compare_results);
    for (size_t m = 0; m < response_imgs; ++m) {
        unsigned long a;
        double b;

        memcpy(&a, res + m * single_res_len, sizeof(unsigned long));
        memcpy(&b, res + m * single_res_len + sizeof(unsigned long), sizeof(double));

        outside_util::printf("%lu %f\n", a, b);
    }
    outside_util::printf("\n");
    ////////////////////

    // fill response buffer
    *out_len = sizeof(size_t) + RESPONSE_DOCS * single_res_len;
    *out = (uint8_t*)malloc(*out_len);
    memset(*out, 0x00, *out_len);

    memcpy(*out, &response_imgs, sizeof(size_t));
    memcpy(*out + sizeof(size_t), res, response_imgs * single_res_len);

    free(res);
}

void extern_lib::process_message(uint8_t **out, size_t *out_len, const uint8_t *in, const size_t in_len) {
    // pass pointer without op char to processing functions
    uint8_t* input = ((uint8_t*)in) + sizeof(unsigned char);
    const size_t input_len = in_len - sizeof(unsigned char);

    //debug_printbuf((uint8_t*)in, in_len);

    *out_len = 0;
    *out = NULL;

    switch(((unsigned char*)in)[0]) {
        case OP_IEE_INIT: {
            outside_util::printf("Init repository!\n");

            unsigned nr_clusters;
            size_t desc_len;

            memcpy(&nr_clusters, input, sizeof(unsigned));
            memcpy(&desc_len, input + sizeof(unsigned), sizeof(size_t));

            outside_util::printf("desc_len %lu %u\n", desc_len, nr_clusters);

            repository_init(nr_clusters, desc_len);
            // TODO server init
            ok_response(out, out_len);
            break;
        }
        case OP_IEE_TRAIN_ADD: {
            outside_util::printf("Train add image!\n");

            unsigned long id;
            size_t nr_desc;

            memcpy(&id, input, sizeof(unsigned long));
            memcpy(&nr_desc, input + sizeof(unsigned long), sizeof(size_t));

            train_add_image(k, id, nr_desc, input, input_len);
            ok_response(out, out_len);
            break;
        }
        case OP_IEE_TRAIN: {
            outside_util::printf("Train kmeans!\n");
            train_kmeans(k);
            ok_response(out, out_len);
            break;
        }
        case OP_IEE_ADD: {
            outside_util::printf("Add after train images!\n");
            add_image(out, out_len, input, input_len);
            ok_response(out, out_len);
            break;
        }
        case OP_IEE_SEARCH: {
            outside_util::printf("Search!\n");
            search_image(out, out_len, input, input_len);
            break;
        }
        case OP_IEE_CLEAR: {
            outside_util::printf("Clear repository!\n");
            repository_clear();
            ok_response(out, out_len);
            break;
        }
        default: {
            outside_util::printf("Unrecognised op: %02x\n", ((unsigned char *) in)[0]);
            break;
        }
    }
}

void extern_lib::init() {
    outside_util::printf("init function!\n");
}
