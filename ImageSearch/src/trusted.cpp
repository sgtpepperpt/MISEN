#include "internal_trusted.h"

#define LABEL_LEN SHA256_OUTPUT_SIZE
#define UNENC_VALUE_LEN (LABEL_LEN + sizeof(unsigned long) + sizeof(unsigned)) // (32 + 8 + 4)
#define ENC_VALUE_LEN (UNENC_VALUE_LEN + 4) // AES padding (44 + 4)

BagOfWordsTrainer* k;
map<std::string, size_t> image_descriptors;

int server_socket;

// keys for server encryption
void* kf;
void* ke;

// store kw for each centre
uint8_t** centre_keys;

// overall counters
unsigned* counters;
unsigned total_docs;

// DEBUG kmeans done count
int ccount = 0;

void normalise_idf(double *pDouble);

void debug_print_points(float* points, size_t nr_rows, size_t row_len) {
    for (size_t y = 0; y < nr_rows; ++y) {
        for (size_t l = 0; l < row_len; ++l)
            sgx_printf("%lf ", *(points + y * row_len + l));
        sgx_printf("\n");
    }
}

void debug_printbuf(uint8_t* buf, size_t len) {
    for (size_t l = 0; l < len; ++l)
        sgx_printf("%02x ", buf[l]);
    sgx_printf("\n");

}

void ok_response(void** out, size_t* out_len) {
    *out_len = 1;
    unsigned char* ret = (unsigned char*)sgx_untrusted_malloc(sizeof(unsigned char));
    ret[0] = 'x';
    *out = ret;
}

void init_repository(void** out, size_t* out_len, const unsigned char* in, const size_t in_len) {
    unsigned nr_clusters;
    size_t desc_len;

    memcpy(&nr_clusters, in, sizeof(unsigned));
    memcpy(&desc_len, in + sizeof(unsigned), sizeof(size_t));

    k = new BagOfWordsTrainer(nr_clusters, desc_len);

    // generate keys
    kf = malloc(SHA256_BLOCK_SIZE);
    ke = malloc(AES_KEY_SIZE);
    c_random(kf, SHA256_BLOCK_SIZE);
    c_random(ke, AES_KEY_SIZE);

    // init key map
    centre_keys = (uint8_t**)malloc(sizeof(uint8_t*) * nr_clusters);
    for (unsigned i = 0; i < nr_clusters; ++i) {
        uint8_t* key = (uint8_t*)malloc(SHA256_OUTPUT_SIZE);
        c_hmac_sha256(key, &i, sizeof(unsigned), kf, SHA256_BLOCK_SIZE);

        centre_keys[i] = key;
    }

    // init counters
    counters = (unsigned*)malloc(nr_clusters * sizeof(unsigned));
    memset(counters, 0x00, nr_clusters * sizeof(unsigned));
    total_docs = 0;

    // establish connection to uee
    server_socket = sgx_open_uee_connection();

    // send generic response to client
    ok_response(out, out_len);
}

void train_add_image(void** out, size_t* out_len, const unsigned char* in, const size_t in_len) {
    unsigned long id;
    size_t nr_rows;

    memcpy(&id, in, sizeof(unsigned long));
    memcpy(&nr_rows, in + sizeof(unsigned long), sizeof(size_t));

    sgx_printf("add: id %lu nr_descs %d\n", id, nr_rows);

    // store id and desc counter in map TODO store in server until kmeans done? so that client does not need to resend?
    /*std::string i_str(id);
    image_descriptors[i_str] = nr_rows;*/

    // store in trainer
    img_descriptor* descriptor = (img_descriptor*)malloc(sizeof(img_descriptor));
    descriptor->count = nr_rows;

    descriptor->buffer = (float*)malloc(nr_rows * k->desc_len() * sizeof(float));
    memcpy(descriptor->buffer, in + sizeof(unsigned long) + sizeof(size_t), nr_rows * k->desc_len() * sizeof(float));

    k->add_descriptors(descriptor);

    if(k->is_full()) {
        sgx_printf("Full, train images! %d\n", ccount++);
        img_descriptor* centres = k->cluster();
        //debug_print_points(centres->buffer, centres->count, k->desc_len());
        free(centres);
    }

    // return ok to client
    ok_response(out, out_len);
}

void train_kmeans(void** out, size_t* out_len, const unsigned char* in, const size_t in_len) {
    img_descriptor* centres = k->cluster();
    //debug_print_points(centres->buffer, centres->count, k->desc_len());
    free(centres);

    // return ok
    ok_response(out, out_len);
}

static unsigned* process_new_image(const uint8_t* in, const size_t in_len) {
    size_t nr_desc;

    memcpy(&nr_desc, in, sizeof(size_t));

    float* descriptors = (float*)(in + sizeof(size_t));
    sgx_printf("add: nr_rows %d\n", nr_desc);

    //size_t to_server_len = sizeof(unsigned char) + ;
    //unsigned char* to_server = (unsigned char*)malloc(to_server * to_server_len);

    unsigned* frequencies = (unsigned*)malloc(k->nr_centres() * sizeof(unsigned));
    memset(frequencies, 0x00, k->nr_centres() * sizeof(unsigned));

    //debug_print_points(descriptors, nr_desc, 64);

    // for each descriptor, calculate its closest centre
    for (size_t i = 0; i < nr_desc; ++i) {
        /*double sum = 0;
        for (int l = 0; l < 64; ++l) {
            sum += *(descriptors + i * k->desc_len() + l);
        }
        sgx_printf("sum %f\n", sum);*/
        double min_dist = DBL_MAX;
        int pos = -1;

        //sgx_printf("descriptor: \n");
        //debug_print_points(descriptors + i * k->desc_len(), 1, 64);

        for (size_t j = 0; j < k->nr_centres(); ++j) {
            //sgx_printf("centre %u: \n", j);
            //debug_print_points(k->get_centre(j), 1, 64);
            double dist_to_centre = normL2Sqr(k->get_centre(j), descriptors + i * k->desc_len(), k->desc_len());
            //sgx_printf("dist %f\n", dist_to_centre);

            if(dist_to_centre < min_dist) {
                //sgx_printf("is min %u\n", j);
                min_dist = dist_to_centre;
                pos = j;
            }
        }

        frequencies[pos]++;
    }

    return frequencies;
}

static void add_image(void** out, size_t* out_len, const uint8_t* in, const size_t in_len) {
    // used for aes TODO better
    unsigned char ctr[AES_BLOCK_SIZE];
    memset(ctr, 0x00, AES_BLOCK_SIZE);

    // get image id from buffer
    unsigned long id;
    memcpy(&id, in, sizeof(unsigned long));
    sgx_printf("add, id %lu\n", id);

    unsigned* frequencies = process_new_image(in + sizeof(unsigned long), in_len - sizeof(unsigned long));

    sgx_printf("frequencies: ");
    for (size_t i = 0; i < k->nr_centres(); i++)
        sgx_printf("%u ", frequencies[i]);
    sgx_printf("\n");

    // prepare request to uee
    size_t req_len = sizeof(unsigned char) + sizeof(size_t) + k->nr_centres() * (LABEL_LEN + ENC_VALUE_LEN);
    uint8_t* req_buffer = (uint8_t*)malloc(req_len);
    req_buffer[0] = 'a';

    uint8_t* tmp = req_buffer + sizeof(unsigned char);

    size_t len = k->nr_centres();
    memcpy(tmp, &len, sizeof(size_t));
    tmp += sizeof(size_t);

    // add visual words to server with their frequencies
    for (size_t m = 0; m < k->nr_centres(); ++m) { //TODO batches
        memset(ctr, 0x00, AES_BLOCK_SIZE);
        // calculate label based on current counter for the centre
        int counter = counters[m];
        c_hmac_sha256(tmp, &counter, sizeof(unsigned), centre_keys[m], LABEL_LEN);
        uint8_t* label = tmp; // keep a reference to the label
        //debug_printbuf(tmp, LABEL_LEN);
        tmp += LABEL_LEN;

        // increase the centre counter, if present
        if(frequencies[m])
            counters[m]++;

        //debug_printbuf(label, LABEL_LEN);

        // calculate value
        // label + img_id + frequency
        unsigned char value[UNENC_VALUE_LEN];
        memcpy(value, label, LABEL_LEN);
        memcpy(value + LABEL_LEN, &id, sizeof(unsigned long));
        memcpy(value + LABEL_LEN + sizeof(unsigned long), &frequencies[m], sizeof(unsigned));

        // encrypt value
        c_encrypt(tmp, value, UNENC_VALUE_LEN, ke, ctr);
        //debug_printbuf(tmp, ENC_VALUE_LEN);
        tmp += ENC_VALUE_LEN;
    }

    // send to server
    size_t res_len;
    void* res;
    sgx_uee_process(server_socket, &res, &res_len, req_buffer, req_len);
    sgx_untrusted_free(res);

    total_docs++;

    free(frequencies);
    free(req_buffer);

    // return ok
    ok_response(out, out_len);
}

static double* calc_idf(size_t total_docs, unsigned* counters) {
    double* idf = (double*)malloc(k->nr_centres() * sizeof(double));
    memset(idf, 0x00, k->nr_centres() * sizeof(double));

    for(size_t i = 0; i < k->nr_centres(); i++) {
        double non_zero_counters = counters[i] ? (double)counters[i] : (double)counters[i] + 1.0;
        idf[i] = log10((double)total_docs / non_zero_counters);
        //sgx_printf("%lu %lu %lf\n", total_docs, (size_t)non_zero_counters, log10((double)total_docs / non_zero_counters));
    }

    return idf;
}

#include <map>

void weight_idf(double *idf, unsigned* frequencies) {
    for (int i = 0; i < k->nr_centres(); ++i)
        idf[i] *= frequencies[i];
}

int compare_results(const void *a, const void *b) {
    double d_a = *((const double*) (a + sizeof(unsigned long)));
    double d_b = *((const double*) (b + sizeof(unsigned long)));

    if (d_a == d_b)
        return 0;
    else
        return d_a < d_b ? 1 : -1;
}

void search_image(void** out, size_t* out_len, const uint8_t* in, const size_t in_len) {
    unsigned* frequencies = process_new_image(in, in_len);
    for (size_t i = 0; i < k->nr_centres(); ++i) {
        sgx_printf("%u ", frequencies[i]);
    } sgx_printf("\n");

    double* idf = calc_idf(total_docs, counters);
    for (size_t i = 0; i < k->nr_centres(); ++i) {
        sgx_printf("%lf ", idf[i]);
    } sgx_printf("\n");

    weight_idf(idf, frequencies);
    for (size_t i = 0; i < k->nr_centres(); ++i) {
        sgx_printf("%lf ", idf[i]);
    } sgx_printf("\n");

/*
    const unsigned max_labels_batch = 10000;
    uint8_t* req_buffer = (uint8_t*)malloc(max_labels_batch * LABEL_LEN);
    memset(req_buffer, 0x00, max_labels_batch * LABEL_LEN);
*/
    // calc size of request
    size_t nr_labels = 0;
    for (size_t i = 0; i < k->nr_centres(); ++i) {
        if(frequencies[i])
            nr_labels += counters[i];
    }

    uint8_t* res_buffer = (uint8_t*)malloc(nr_labels * LABEL_LEN);
    memset(res_buffer, 0x00, nr_labels * LABEL_LEN);

    //uint8_t* tmp = res_buffer;

    std::map<unsigned long, double> docs;

    // TODO randomise, batch_requests, do not ignore zero counters
    for (size_t i = 0; i < k->nr_centres(); ++i) {
        if(!frequencies[i])
            continue;

        sgx_printf("centre %lu (%u counters)\n", i, counters[i]);

        size_t req_len = sizeof(unsigned char) + sizeof(size_t) + counters[i] * LABEL_LEN;
        uint8_t* req_buffer = (uint8_t*)malloc(req_len);
        req_buffer[0] = 's';
        uint8_t* tmp = req_buffer + sizeof(unsigned char);

        size_t count = counters[i];
        memcpy(tmp, &count, sizeof(size_t));
        tmp += sizeof(size_t);

        // iterate over all the occurences of that centre
        for (unsigned j = 0; j < counters[i]; ++j) {
            // calc label
            uint8_t req[sizeof(unsigned char) + LABEL_LEN];
            req[0] = 's';

            c_hmac_sha256(tmp, &j, sizeof(unsigned), centre_keys[i], LABEL_LEN);
            tmp += LABEL_LEN;
            //debug_printbuf(req + 1, LABEL_LEN);
        }

        size_t res_len;
        uint8_t* res_buffer;

        // perform request to uee
        sgx_uee_process(server_socket, (void**)&res_buffer, &res_len, req_buffer, req_len);

        uint8_t* res_tmp = res_buffer;

        for (unsigned j = 0; j < counters[i]; ++j) {
            // decode res
            uint8_t res_unenc[UNENC_VALUE_LEN];
            unsigned char ctr[AES_BLOCK_SIZE];
            memset(ctr, 0x00, AES_BLOCK_SIZE);

            c_decrypt(res_unenc, res_tmp, ENC_VALUE_LEN, ke, ctr);
            res_tmp += ENC_VALUE_LEN;

            /*sgx_printf("------------\n");
            debug_printbuf(req + sizeof(unsigned char), LABEL_LEN);
            debug_printbuf(res_unenc, UNENC_VALUE_LEN);
            sgx_printf("------------\n");*/

            // TODO verify label
            unsigned long id;
            memcpy(&id, res_unenc + LABEL_LEN, sizeof(unsigned long));

            unsigned frequency;
            memcpy(&frequency, res_unenc + LABEL_LEN + sizeof(unsigned long), sizeof(unsigned));

            //sgx_printf("id: %lu %u\n", id, frequency);

            if (docs[id])
                docs[id] += frequency * idf[i];
            else
                docs[id] = frequency * idf[i];

            //sgx_printf("tf idf %lf\n", docs[id]);

        }

        free(req_buffer);
        sgx_untrusted_free(res_buffer);
    }

    // put result in simple structure, to sort
    uint8_t* res = (uint8_t*)malloc(docs.size() * (sizeof(unsigned long) + sizeof(double)));
    int pos = 0;
    for (map<unsigned long, double>::iterator l = docs.begin(); l != docs.end() ; ++l) {
        memcpy(res + pos * (sizeof(unsigned long) + sizeof(double)), &l->first, sizeof(unsigned long));
        memcpy(res + pos * (sizeof(unsigned long) + sizeof(double)) + sizeof(unsigned long), &l->second, sizeof(double));
        pos++;
    }

    qsort(res, docs.size(), sizeof(unsigned long) + sizeof(double), compare_results);
    for (size_t m = 0; m < min(docs.size(), 15); ++m) {
        unsigned long a;
        double b;

        memcpy(&a, res + m * (sizeof(unsigned long) + sizeof(double)), sizeof(unsigned long));
        memcpy(&b, res + m * (sizeof(unsigned long) + sizeof(double)) + sizeof(unsigned long), sizeof(double));

        sgx_printf("%lu %f\n", a, b);
    } sgx_printf("\n");

    free(res);

    free(res_buffer);
    free(idf);
    free(frequencies);

    ok_response(out, out_len);
}

void clear_repository(void** out, size_t* out_len, const unsigned char* in, const size_t in_len) {
    free(kf);
    free(ke);

    for (size_t i = 0; i < k->nr_centres(); i++)
        free(centre_keys[i]);
    free(centre_keys);

    free(counters);

    // cleanup bag-of-words class
    k->cleanup();
    free(k);

    sgx_close_uee_connection(server_socket);

    // return ok
    ok_response(out, out_len);
}

void trusted_process_message(void** out, size_t* out_len, const void* in, const size_t in_len) {
    // decrypt in


    // pass pointer without op char to processing functions
    unsigned char* input = ((unsigned char*)in) + sizeof(unsigned char);
    const size_t input_len = in_len - sizeof(unsigned char);

    *out_len = 0;
    *out = NULL;

    switch(((unsigned char*)in)[0]) {
        case 'i':
            sgx_printf("Init repository!\n");
            init_repository(out, out_len, input, input_len);
            break;
        case 'a':
            sgx_printf("Train add image!\n");
            train_add_image(out, out_len, input, input_len);
            break;
        case 'k':
            sgx_printf("Train kmeans!\n");
            train_kmeans(out, out_len, input, input_len);
            break;
        case 'n':
            sgx_printf("Add after train images!\n");
            add_image(out, out_len, input, input_len);
            break;
        case 's':
            sgx_printf("Search!\n");
            search_image(out, out_len, input, input_len);
            break;
        case 'c':
            sgx_printf("Clear repository!\n");
            clear_repository(out, out_len, input, input_len);
            break;
        default:
            sgx_printf("Unrecognised op: %c\n", ((unsigned char*)in)[0]);
            break;
    }

    // encrypt out

}
