#include <unistd.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <map>

// includes from framework
#include "definitions.h"
#include "outside_util.h"
#include "trusted_crypto.h"
#include "trusted_util.h"
#include "extern_lib.h" // defines the functions we implement here

// internal
#include "imgsrc_definitions.h"
#include "parallel.h"
#include "scoring.h"
#include "repository.h"
#include "img_processing.h"
#include "util.h"
#include "training/lsh/lsh.h"

using namespace std;

repository* r = NULL;

#if PARALLEL_ADD_IMG
void* add_img_parallel(void* args) {
    img_add_args* arg = (img_add_args*)args;

    // used for aes TODO better
    unsigned char ctr[AES_BLOCK_SIZE];
    memset(ctr, 0x00, AES_BLOCK_SIZE);

    // prepare request to uee
    r->resource_pool[arg->tid].add_req_buffer[0] = OP_UEE_ADD;

    size_t to_send = arg->centre_pos_end - arg->centre_pos_start + 1;
    size_t centre_pos = arg->centre_pos_start;
    while (to_send > 0) {
        size_t batch_len = min(ADD_MAX_BATCH_LEN, to_send);

        // add number of pairs to buffer
        uint8_t* tmp = r->resource_pool[arg->tid].add_req_buffer + sizeof(unsigned char);
        memcpy(tmp, &batch_len, sizeof(size_t));
        tmp += sizeof(size_t);

        // add all batch pairs to buffer
        for (size_t i = 0; i < batch_len; ++i) {
            memset(ctr, 0x00, AES_BLOCK_SIZE);

            // calculate label based on current counter for the centre
            unsigned counter = r->counters[centre_pos];
            //outside_util::printf("(%lu/%lu/%lu) counter %u\n", arg->centre_pos_start, centre_pos, arg->centre_pos_end, counter);
            //outside_util::printf("%p %u\n", centre_keys[centre_pos], SHA256_OUTPUT_SIZE);
            tcrypto::hmac_sha256(tmp, &counter, sizeof(unsigned), r->centre_keys[centre_pos], SHA256_OUTPUT_SIZE);
            uint8_t* label = tmp; // keep a reference to the label
            tmp += LABEL_LEN;

            // increase the centre's counter, if present
            if (arg->frequencies[centre_pos])//TODO remove this if, counter is increased even if 0
                ++r->counters[centre_pos];

            // calculate value
            // label + img_id + frequency
            unsigned char value[UNENC_VALUE_LEN];
            memcpy(value, label, LABEL_LEN);
            memcpy(value + LABEL_LEN, &arg->id, sizeof(unsigned long));
            memcpy(value + LABEL_LEN + sizeof(unsigned long), &arg->frequencies[centre_pos], sizeof(unsigned));

            // encrypt value
            tcrypto::encrypt(tmp, value, UNENC_VALUE_LEN, r->ke, ctr);
            tmp += ENC_VALUE_LEN;

            to_send--;
            centre_pos++;
        }

        // send batch to server
        size_t res_len;
        void* res;
        outside_util::uee_process(r->resource_pool[arg->tid].server_socket, &res, &res_len, r->resource_pool[arg->tid].add_req_buffer, sizeof(unsigned char) + sizeof(size_t) + batch_len * PAIR_LEN);
        outside_util::outside_free(res); // discard ok response
    }

    return NULL;
}
#endif

static void add_image(const unsigned long id, const size_t nr_desc, float* descriptors) {
    // used for aes TODO better
    unsigned char ctr[AES_BLOCK_SIZE];
    memset(ctr, 0x00, AES_BLOCK_SIZE);

    //const unsigned* frequencies = process_new_image(r->k, nr_desc, descriptors);
    const unsigned* frequencies = calc_freq(r->lsh, descriptors, nr_desc, r->cluster_count);
    /*outside_util::printf("frequencies: ");
    for (size_t i = 0; i < r->k->nr_centres(); i++) {
        if (frequencies[i])
            outside_util::printf("%lu -> %u\n", i, frequencies[i]);
    }
    outside_util::printf("\n");*/

#if PARALLEL_ADD_IMG
    const unsigned nr_threads = trusted_util::thread_get_count();
    size_t k_per_thread = r->k->nr_centres() / nr_threads;

    img_add_args* args = (img_add_args*)malloc(nr_threads * sizeof(img_add_args));
    for (unsigned j = 0; j < nr_threads; ++j) {
        // each thread receives the generic pointers and the thread ranges
        args[j].id = id;
        args[j].frequencies = (unsigned*)frequencies;
        args[j].tid = j;

        if(j == 0) {
            args[j].centre_pos_start = 0;
            args[j].centre_pos_end = k_per_thread;
        } else {
            args[j].centre_pos_start = j * k_per_thread + 1;

            if (j + 1 == nr_threads)
                args[j].centre_pos_end = r->k->nr_centres() - 1;// TODO -1 because still not [a,b[
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
    size_t max_req_len = sizeof(unsigned char) + sizeof(size_t) + ADD_MAX_BATCH_LEN * PAIR_LEN;
    uint8_t* req_buffer = (uint8_t*)malloc(max_req_len);
    req_buffer[0] = OP_UEE_ADD;

    // TODO should send r->k->nr_centres()?? hides how many real descs, but also more weight for ocalls
    size_t nr_labels = 0;
    for (int j = 0; j < r->k->nr_centres(); ++j) {
        if(frequencies[j])
            nr_labels++;
    }

    size_t to_send = nr_labels;//r->k->nr_centres();
    size_t centre_pos = 0;
    while (to_send > 0) {
        memset(req_buffer, 0x00, max_req_len);
        req_buffer[0] = OP_UEE_ADD;

        size_t batch_len = min(ADD_MAX_BATCH_LEN, to_send);

        // add number of pairs to buffer
        uint8_t* tmp = req_buffer + sizeof(unsigned char);
        memcpy(tmp, &batch_len, sizeof(size_t));
        tmp += sizeof(size_t);

        // add all batch pairs to buffer
        for (size_t i = 0; i < batch_len; ++i) {
            memset(ctr, 0x00, AES_BLOCK_SIZE);

            // calculate label based on current counter for the centre
            int counter = r->counters[centre_pos];
            tcrypto::hmac_sha256(tmp, &counter, sizeof(unsigned), r->centre_keys[centre_pos], LABEL_LEN);
            uint8_t* label = tmp; // keep a reference to the label
            tmp += LABEL_LEN;

            /*if (label[0] == 0x66 && label[1] == 0xF8) {// TODO due to the empty centroids bug
                //outside_util::printf("original\n");
                for (size_t l = 0; l < LABEL_LEN; ++l)
                    outside_util::printf("%02x ", (tmp - LABEL_LEN)[l]);
                outside_util::printf("\n");
            }*/

            // increase the centre's counter
            ++r->counters[centre_pos];

            // calculate value
            // label + img_id + frequency
            unsigned char value[UNENC_VALUE_LEN];
            memcpy(value, label, LABEL_LEN);
            memcpy(value + LABEL_LEN, &id, sizeof(unsigned long));
            memcpy(value + LABEL_LEN + sizeof(unsigned long), &frequencies[centre_pos], sizeof(unsigned));

            // encrypt value
            uint8_t nonce[crypto_secretbox_NONCEBYTES];
            memset(nonce, 0x00, crypto_secretbox_NONCEBYTES);
            tcrypto::encrypt(tmp, value, UNENC_VALUE_LEN, r->ke, ctr);
            tmp += ENC_VALUE_LEN;

            to_send--;

            do {
                centre_pos++;
            } while(!frequencies[centre_pos]);
        }

        /*for (int j = 0; j < sizeof(unsigned char) + sizeof(size_t) + batch_len * PAIR_LEN; ++j) {
            if(j < sizeof(unsigned char) + sizeof(size_t) + batch_len * PAIR_LEN - 2 && req_buffer[j] == 0xAE && req_buffer[j+1] == 0xD8 && req_buffer[j+2] == 0xD2) {
                outside_util::printf("this has it %d\n", j - sizeof(unsigned char) - sizeof(size_t));
            }
        }*/

        // send batch to server
        size_t res_len;
        void* res;
        outside_util::uee_process(r->server_socket, &res, &res_len, req_buffer,
                                  sizeof(unsigned char) + sizeof(size_t) + batch_len * PAIR_LEN);
        outside_util::outside_free(res); // discard ok response
    }

    free(req_buffer);
#endif

    r->total_docs++;
    free((void*)frequencies);
    outside_util::printf("--------------------\n");
}

typedef struct img_pair {
    size_t img_id;
    unsigned frequency;
} img_pair;

void search_image(uint8_t** out, size_t* out_len, const size_t nr_desc, float* descriptors) {
    using namespace outside_util;

    //const unsigned* frequencies = process_new_image(r->k, nr_desc, descriptors);
    const unsigned* frequencies = calc_freq(r->lsh, descriptors, nr_desc, r->cluster_count);
    /*for (size_t i = 0; i < r->k->nr_centres(); ++i) {
        if(frequencies[i])
            outside_util::printf("%lu -> %u; ", i, frequencies[i]);
    }
    outside_util::printf("\n");*/

    double* idf = calc_idf(r->total_docs, r->counters, r->k->nr_centres());
    /*for (size_t i = 0; i < r->k->nr_centres(); ++i) {
        outside_util::printf("%lf ", idf[i]);
    }
    outside_util::printf("\n");*/

    //weight_idf(idf, frequencies);
    /*for (size_t i = 0; i < r->k->nr_centres(); ++i) {
        outside_util::printf("%lf ", idf[i]);
    }
    outside_util::printf("\n");*/

    // calc size of request; ie sum of counters (for all centres of searched image)
    vector<size_t> centroids_to_get;

    size_t nr_labels = 0;
    for (size_t i = 0; i < r->k->nr_centres(); ++i) {
        if (frequencies[i]) {
            nr_labels += r->counters[i];
            centroids_to_get.push_back(i);
        }
    }

    map<size_t, vector<img_pair>> centroids_result;

    // will hold result
    map<unsigned long, double> scores;

    // prepare request to uee
    size_t max_req_len = sizeof(unsigned char) + sizeof(size_t) + SEARCH_MAX_BATCH_LEN * LABEL_LEN;
    uint8_t* req_buffer = (uint8_t*)malloc(max_req_len);
    req_buffer[0] = OP_UEE_SEARCH;

    size_t to_send = nr_labels;
    unsigned centroid_pos = 0;
    unsigned counter_of_centroid_pos = 0;

    unsigned batch_counter = 0;
    while (to_send > 0) {
        unsigned init_centroid_pos = centroid_pos;
        unsigned init_counter_of_centroid_pos = counter_of_centroid_pos;

        memset(req_buffer, 0x00, max_req_len);
        req_buffer[0] = OP_UEE_SEARCH;

        //outside_util::printf("(%02u) centre %lu; counter %lu \n", batch_counter, centre_pos, counter_pos);
        batch_counter++;

        size_t batch_len = min(SEARCH_MAX_BATCH_LEN, to_send);

        // add number of labels to buffer
        uint8_t* tmp = req_buffer + sizeof(unsigned char);
        memcpy(tmp, &batch_len, sizeof(size_t));
        tmp += sizeof(size_t);

        // add all batch labels to buffer
        for (size_t i = 0; i < batch_len; ++i) {
            size_t curr_centroid = centroids_to_get[centroid_pos];
            //outside_util::printf("req %lu %u\n", curr_centroid, counter_of_centroid_pos);

            // calc label
            tcrypto::hmac_sha256(tmp, &counter_of_centroid_pos, sizeof(unsigned), r->centre_keys[curr_centroid], LABEL_LEN);
            tmp += LABEL_LEN;

            //outside_util::printf("request\n");
            /*for (size_t l = 0; l < LABEL_LEN; ++l)
                outside_util::printf("%02x ", (tmp-LABEL_LEN)[l]);
            outside_util::printf("\n");*/

            // update pointers
            ++counter_of_centroid_pos;
            if (counter_of_centroid_pos >= r->counters[curr_centroid]) {
                // search for the next centre where the searched image's frequency is non-zero
                //while (!frequencies[++centre_pos]); // TODO there is a bug when the query image has a centroid that has counter 0, ie with no descriptor previously assigned to it
                counter_of_centroid_pos = 0;
                centroid_pos++;
            }
        }

        to_send -= batch_len;

        // perform request to uee
        size_t res_len;
        uint8_t* res;
        uee_process(r->server_socket, (void**)&res, &res_len, req_buffer, sizeof(unsigned char) + sizeof(size_t) + batch_len * LABEL_LEN);
        uint8_t* res_tmp = res;

        centroid_pos = init_centroid_pos;
        counter_of_centroid_pos = init_counter_of_centroid_pos;

        // process all answers
        for (size_t i = 0; i < batch_len; ++i) {
            // decode res
            uint8_t res_unenc[UNENC_VALUE_LEN];
            unsigned char ctr[AES_BLOCK_SIZE];
            memset(ctr, 0x00, AES_BLOCK_SIZE);

            tcrypto::decrypt(res_unenc, res_tmp, ENC_VALUE_LEN, r->ke, ctr);
            res_tmp += ENC_VALUE_LEN;

            int verify = memcmp(res_unenc, (req_buffer + sizeof(unsigned char) + sizeof(size_t)) + i * LABEL_LEN, LABEL_LEN);
            /*for (size_t l = 0; l < LABEL_LEN; ++l)
                outside_util::printf("%02x ", res_unenc[l]);
            outside_util::printf("\n");

            for (size_t l = 0; l < LABEL_LEN; ++l)
                outside_util::printf("%02x ", ((req_buffer + sizeof(unsigned char) + sizeof(size_t)) + i * LABEL_LEN)[l]);
            outside_util::printf("\n");*/

            if (verify) {
                outside_util::printf("Label verification doesn't match! Exit\n");
                outside_util::exit(-1);
            }

            size_t curr_centroid = centroids_to_get[centroid_pos];

            unsigned long id;
            memcpy(&id, res_unenc + LABEL_LEN, sizeof(unsigned long));

            unsigned frequency;
            memcpy(&frequency, res_unenc + LABEL_LEN + sizeof(unsigned long), sizeof(unsigned));

            /*img_pair p;
            p.frequency = frequency;
            p.img_id = id;
            centroids_result[curr_centroid].push_back(p);*/

            //outside_util::printf("res %lu %u\n", curr_centroid, counter_of_centroid_pos);
            /*outside_util::printf("frequency of %lu: %u\n", curr_centroid, frequencies[curr_centroid]);
            outside_util::printf("frequency %u %f\n", frequency, idf[curr_centroid]);*/

            if(!scores[id])
                scores[id] = frequency ? frequencies[curr_centroid] * (1 + log10(frequency)) * idf[curr_centroid] : 0;
            else
                scores[id] += frequency ? frequencies[curr_centroid] * (1 + log10(frequency)) * idf[curr_centroid] : 0;

            // update pointers
            ++counter_of_centroid_pos;
            if (counter_of_centroid_pos >= r->counters[curr_centroid]) {
                counter_of_centroid_pos = 0;
                centroid_pos++;
            }
        }

        outside_util::outside_free(res);
    }

    for(auto x = scores.begin(); x != scores.end(); x++ ) {
        outside_util::printf("%lu %f\n", x->first, x->second);
    }

    free(idf);
    free((void*)frequencies);

   /* vector<unsigned long> imgs;
    for (auto it = centroids_result.begin(); it != centroids_result.end(); it++) {
        size_t centroid = it->first;
        vector<img_pair> pairs = it->second;


    }*/

    // TODO randomise, do not ignore zero counters, fix batch search for 2000

    // calculate score and respond to client
    const unsigned response_imgs = min(scores.size(), RESPONSE_DOCS);
    const size_t single_res_len = sizeof(unsigned long) + sizeof(double);

    // put result in simple structure, to sort
    uint8_t* res;

    ////////////////////
    //sort_docs(docs, response_imgs, &res); // TODO
    res = (uint8_t*)malloc(scores.size() * single_res_len);
    int pos = 0;
    for (map<unsigned long, double>::iterator l = scores.begin(); l != scores.end(); ++l) {
        memcpy(res + pos * single_res_len, &l->first, sizeof(unsigned long));
        memcpy(res + pos * single_res_len + sizeof(unsigned long), &l->second, sizeof(double));
        pos++;
    }

    qsort(res, scores.size(), single_res_len, compare_results);
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

void extern_lib::process_message(uint8_t** out, size_t* out_len, const uint8_t* in, const size_t in_len) {
    // pass pointer without op char to processing functions
    uint8_t* input = ((uint8_t*)in) + sizeof(unsigned char);
    const size_t input_len = in_len - sizeof(unsigned char);

    //debug_printbuf((uint8_t*)in, in_len);

    *out_len = 0;
    *out = NULL;

    switch (((unsigned char*)in)[0]) {
        case OP_IEE_INIT: {
            outside_util::printf("Init repository!\n");

            unsigned nr_clusters;
            size_t desc_len;

            memcpy(&nr_clusters, input, sizeof(unsigned));
            memcpy(&desc_len, input + sizeof(unsigned), sizeof(size_t));

            outside_util::printf("desc_len %lu %u\n", desc_len, nr_clusters);

            r = repository_init(nr_clusters, desc_len);
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

            train_add_image(r->k, id, nr_desc, input, input_len);
            ok_response(out, out_len);
            break;
        }
        case OP_IEE_TRAIN_LSH: {
            outside_util::printf("Train lsh!\n");
            r->lsh = init_lsh(r->cluster_count);
            outside_util::printf("Trained lsh!\n");
            ok_response(out, out_len);
            break;
        }
        case OP_IEE_ADD_LSH: {
            outside_util::printf("Add lsh images!\n");

            // get image (id, nr_desc, descriptors) from buffer
            unsigned long id;
            memcpy(&id, input, sizeof(unsigned long));
            outside_util::printf("add, id %lu\n", id);

            size_t nr_desc;
            memcpy(&nr_desc, input + sizeof(unsigned long), sizeof(size_t));

            float* descriptors = (float*)(input + sizeof(unsigned long) + sizeof(size_t));

            add_image(id, nr_desc, descriptors);
            ok_response(out, out_len);
            break;
        }
        case OP_IEE_TRAIN: {
            outside_util::printf("Train kmeans!\n");
            train_kmeans(r->k);
            ok_response(out, out_len);
            break;
        }
        case OP_IEE_ADD: {
            outside_util::printf("Add after train images!\n");

            // get image (id, nr_desc, descriptors) from buffer
            unsigned long id;
            memcpy(&id, input, sizeof(unsigned long));
            outside_util::printf("add, id %lu\n", id);

            size_t nr_desc;
            memcpy(&nr_desc, input + sizeof(unsigned long), sizeof(size_t));

            float* descriptors = (float*)(input + sizeof(unsigned long) + sizeof(size_t));

            add_image(id, nr_desc, descriptors);
            ok_response(out, out_len);
            break;
        }
        case OP_IEE_SEARCH: {
            outside_util::printf("Search!\n");

            size_t nr_desc;
            memcpy(&nr_desc, input, sizeof(size_t));

            float* descriptors = (float*)(input + sizeof(size_t));

            search_image(out, out_len, nr_desc, descriptors);
            break;
        }
        case OP_IEE_CLEAR: {
            outside_util::printf("Clear repository!\n");
            repository_clear(r);
            r = NULL;
            ok_response(out, out_len);
            break;
        }
        case OP_IEE_LOAD: {
            train_kmeans_load(r->k);
            break;
        }
        case OP_IEE_READ_MAP: {
            size_t res_len;
            uint8_t* res;
            const unsigned char op = OP_UEE_READ_MAP;

            outside_util::uee_process(r->server_socket, (void**)&res, &res_len, &op, sizeof(unsigned char));
            outside_util::outside_free(res);

            // read iee-side data securely from disc
            void* file = trusted_util::open_secure("iee_data", "rb");
            if (!file) {
                outside_util::printf("Could not read data file\n");
                outside_util::exit(1);
            }

            trusted_util::read_secure(&(r->total_docs), sizeof(unsigned), 1, file);
            trusted_util::read_secure(r->counters, sizeof(unsigned) * r->k->nr_centres(), 1, file);
            trusted_util::close_secure(file);

            ok_response(out, out_len);
            break;
        }
        case OP_IEE_WRITE_MAP: {
            size_t res_len;
            uint8_t* res;
            const unsigned char op = OP_UEE_WRITE_MAP;

            outside_util::uee_process(r->server_socket, (void**)&res, &res_len, &op, sizeof(unsigned char));
            outside_util::outside_free(res);

            // write iee-side data securely to disc
            void* file = trusted_util::open_secure("iee_data", "wb");
            if (!file) {
                outside_util::printf("Could not write data file\n");
                outside_util::exit(1);
            }

            trusted_util::write_secure(&(r->total_docs), sizeof(unsigned), 1, file);
            trusted_util::write_secure(r->counters, sizeof(unsigned) * r->k->nr_centres(), 1, file);
            trusted_util::close_secure(file);

            ok_response(out, out_len);
            break;
        }
        case OP_IEE_SET_CODEBOOK: {
            //float* centres = (float*)malloc(r->cluster_count * r->desc_len * sizeof(float));
            //memcpy(centres, input, r->cluster_count * r->desc_len * sizeof(float));
            //train_kmeans_set(r->k, centres, 0);

            r->lsh = (float**)malloc(sizeof(float*) * r->cluster_count);
            for (int i = 0; i < r->cluster_count; ++i) {
                float* p = (float*)malloc(r->desc_len * sizeof(float));
                memcpy(p, input + i * (r->desc_len * sizeof(float)), r->desc_len * sizeof(float));
                r->lsh[i] = p;

                /*for (size_t l = 0; l < r->desc_len; ++l)
                    outside_util::printf("%f ", p[l]);
                outside_util::printf("\n");*/
            }

            ok_response(out, out_len);
            break;
        }
        default: {
            outside_util::printf("Unrecognised op: %02x\n", ((unsigned char*)in)[0]);
            break;
        }
    }
}

void extern_lib::init() {
    outside_util::printf("init function!\n");
}
