#include "internal_trusted.h"

#define LABEL_LEN SHA256_OUTPUT_SIZE

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

void debug_print_points(float* points, size_t nr_rows, size_t row_len) {
    for (size_t y = 0; y < nr_rows; ++y) {
        for (size_t l = 0; l < row_len; ++l)
            sgx_printf("%lf ", *(points + y * row_len + l));
        sgx_printf("\n");
    }
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

    descriptor->buffer = (float *)malloc(nr_rows * k->desc_len() * sizeof(float));
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

    // for each descriptor, calculate its closest centre
    for (size_t i = 0; i < nr_desc; ++i) {
        double min_dist = DBL_MAX;
        unsigned pos = -1;

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
    unsigned long id;
    memcpy(&id, in, sizeof(unsigned long));
    sgx_printf("add: %lu\n", id);

    unsigned* frequencies = process_new_image(in + sizeof(unsigned long), in_len - sizeof(unsigned long));

    for (size_t i = 0; i < k->nr_centres(); i++) {
        sgx_printf("%u ", frequencies[i]);
        if(frequencies[i])
            counters[i]++;
    }
    sgx_printf("\n");

    // add visual words to server with their frequencies
    for (size_t m = 0; m < k->nr_centres(); ++m) { //TODO batches
        // calculate label based on current counter for the centre
        void* label = malloc(SHA256_OUTPUT_SIZE);
        int counter = counters[m];
        c_hmac_sha256(label, &counter, sizeof(unsigned), centre_keys[m], SHA256_OUTPUT_SIZE);

        // calculate value
        // label + img_id + frequency
        const size_t value_size = SHA256_OUTPUT_SIZE + sizeof(unsigned long) + sizeof(unsigned);
        unsigned char value[value_size];
        memcpy(value, label, SHA256_OUTPUT_SIZE);
        memcpy(value + SHA256_OUTPUT_SIZE, &id, sizeof(unsigned long));
        memcpy(value + SHA256_OUTPUT_SIZE + sizeof(unsigned long), &frequencies[m], sizeof(unsigned));

        // encrypt value
        const size_t blocks = value_size / AES_BLOCK_SIZE + (value_size % AES_BLOCK_SIZE? 1 : 0);

        unsigned char enc_val[blocks * AES_BLOCK_SIZE];
        unsigned char ctr[AES_BLOCK_SIZE];
        memset(ctr, 0x00, AES_BLOCK_SIZE);
        c_encrypt(enc_val, value, value_size, ke, ctr);

        // send to server
        size_t query_size = sizeof(unsigned char) + SHA256_OUTPUT_SIZE + blocks * AES_BLOCK_SIZE;
        uint8_t query[query_size];
        query[0] = 'a';
        memcpy(query + sizeof(unsigned char), label, SHA256_OUTPUT_SIZE);
        memcpy(query + sizeof(unsigned char) + SHA256_OUTPUT_SIZE, enc_val, AES_BLOCK_SIZE);

        size_t res_len;
        void* res;
        sgx_uee_process(server_socket, &res, &res_len, query, query_size);
        sgx_untrusted_free(res);

        free(label);
    }

    total_docs++;

    free(frequencies);

    // return ok
    ok_response(out, out_len);
}

static double* calc_idf(size_t total_docs, unsigned* counters) {
    double* idf = (double*)malloc(k->nr_centres() * sizeof(double));
    memset(idf, 0x00, k->nr_centres() * sizeof(double));

    for(size_t i = 0; i < k->nr_centres(); i++) {
        idf[i] = log10((double)total_docs / (((double)counters[i]) + 1));
        sgx_printf("%lu %lu %lf\n", total_docs, counters[i], log10((double)total_docs / (((double)counters[i]) + 1)));
    }

    return idf;
}

void search_image(void** out, size_t* out_len, const uint8_t* in, const size_t in_len) {
    unsigned* frequencies = process_new_image(in, in_len);

    double* idf = calc_idf(total_docs, counters);
    for (size_t i = 0; i < k->nr_centres(); ++i) {
        sgx_printf("%lf ", idf[i]);
    }sgx_printf("\n");

    // get documents from server
    for (size_t i = 0; i < k->nr_centres(); ++i) {
        if(!frequencies[i])
            continue;

        const unsigned max_labels_batch = 10000;
        uint8_t* buffer_req = (uint8_t*)malloc(max_labels_batch * LABEL_LEN);
    }

    free(idf);
    free(frequencies);
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
