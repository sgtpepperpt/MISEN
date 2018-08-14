#include "Client.h"

#include "untrusted_util.h"
#include "untrusted_util_tls.h"
#include "definitions.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sodium.h>
#include <random>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <libconfig.h>

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/certs.h"

#include "ImageSearch.h"
#include "client_training.h"

#include "rbisen/SseClient.hpp"

extern "C" {
#include "rbisen/Utils.h"
#include "rbisen/types.h"
}

using namespace cv;
using namespace cv::xfeatures2d;
using namespace std;

static void my_debug(void* ctx, int level, const char* file, int line, const char* str) {
    ((void)level);
    fprintf((FILE*)ctx, "%s:%04d: %s", file, line, str);
    fflush((FILE*)ctx);
}

void bisen_setup(mbedtls_ssl_context ssl, SseClient* client) {
    unsigned char* data_bisen;
    unsigned long long data_size_bisen;

    // setup
    data_size_bisen = client->generate_setup_msg(&data_bisen);
    iee_comm(&ssl, data_bisen, data_size_bisen);
    free(data_bisen);
}

void bisen_update(mbedtls_ssl_context ssl, SseClient* client, unsigned bisen_nr_docs, char* bisen_doc_type) {
    unsigned char* data_bisen;
    unsigned long long data_size_bisen;

    const char* dataset_dir = getenv("DATASET_DIR");
    if (!dataset_dir) {
        printf("DATASET_DIR not defined!\n");
        exit(1);
    }

    // get list of docs for test
    vector<string> doc_paths;
    client->listTxtFiles(dataset_dir, doc_paths);

    size_t nr_updates = 0;
    for (const string doc : doc_paths) {
        vector<map<string, int>> docs;

        if(!strcmp(bisen_doc_type, "wiki")) {
            // extract keywords from a 1M, multiple article, file
            docs = client->extract_keywords_frequency_wiki(dataset_dir + doc);
        } else if(!strcmp(bisen_doc_type, "normal")) {
            // one file is one document
            docs.push_back(client->extract_keywords_frequency(dataset_dir + doc));
        } else {
            printf("Document type not recognised\n");
            exit(0);
        }

        nr_updates += docs.size();

        for (const map<string, int> text : docs) {
            // generate the byte* to send to the server
            data_size_bisen = client->add_new_document(text, &data_bisen);
            iee_comm(&ssl, data_bisen, data_size_bisen);
            free(data_bisen);
        }

        if (nr_updates > bisen_nr_docs) {
            printf("breaking\n");
            break;
        }
    }
}

void bisen_search(mbedtls_ssl_context ssl, SseClient* client, vector<string> queries) {
    unsigned char* data_bisen;
    unsigned long long data_size_bisen;

    for (unsigned k = 0; k < queries.size(); k++) {
        string query = queries[k];
        printf("\n----------------------------\n");
        printf("Query %d: %s\n", k, query.c_str());

        data_size_bisen = client->search(query, &data_bisen);
        iee_send(&ssl, data_bisen, data_size_bisen);

        uint8_t* bisen_out;
        size_t bisen_out_len;
        iee_recv(&ssl, &bisen_out, &bisen_out_len);

        unsigned n_docs;
        memcpy(&n_docs, bisen_out, sizeof(int));
        printf("Number of docs: %d\n", n_docs);

        for (unsigned i = 0; i < n_docs; ++i) {
            int d;
            double s;
            memcpy(&d, bisen_out + sizeof(unsigned) + i * (sizeof(int) + sizeof(double)), sizeof(int));
            memcpy(&s, bisen_out + sizeof(unsigned) + i * (sizeof(int) + sizeof(double)) + sizeof(int), sizeof(double));
            printf("%d %f\n", d, s);
        }

        free(bisen_out);
    }
}

void visen_setup(mbedtls_ssl_context ssl, size_t desc_len, unsigned visen_nr_clusters) {
    size_t in_len = 0;
    uint8_t* in = NULL;

    init(&in, &in_len, visen_nr_clusters, desc_len);
    iee_comm(&ssl, in, in_len);
    free(in);
}

void visen_train_client_kmeans(mbedtls_ssl_context ssl, size_t desc_len, unsigned visen_nr_clusters, char* visen_train_mode, char* visen_centroids_file, Ptr<SIFT> extractor) {
    size_t in_len = sizeof(uint8_t) + visen_nr_clusters * desc_len * sizeof(float);
    uint8_t* in = (uint8_t*)malloc(sizeof(uint8_t) + visen_nr_clusters * desc_len * sizeof(float));
    in[0] = OP_IEE_SET_CODEBOOK_CLIENT_KMEANS;

    float* centroids;
    if(!strcmp(visen_train_mode, "train")) {
        centroids = client_train("/home/guilherme/Datasets/inria/", visen_nr_clusters, desc_len, extractor);

        FILE* file = fopen(visen_centroids_file, "wb");
        if (!file) {
            printf("Could not write centroids file\n");
            exit(1);
        }

        fwrite(centroids, visen_nr_clusters, desc_len * sizeof(float), file);
        fclose(file);
    } else if(!strcmp(visen_train_mode, "load")) {
        centroids = (float*)malloc(visen_nr_clusters * desc_len * sizeof(float));
        FILE* file = fopen(visen_centroids_file, "rb");
        if (!file) {
            printf("Could not read centroids file\n");
            exit(1);
        }

        fread(centroids, visen_nr_clusters, desc_len * sizeof(float), file);
        fclose(file);
    } else {
        printf("error in train mode\n");
        exit(1);
    }

    memcpy(in + 1, centroids, visen_nr_clusters * desc_len * sizeof(float));
    iee_comm(&ssl, in, in_len);
    free(in);
    free(centroids);
}

void visen_train_iee_kmeans(mbedtls_ssl_context ssl, char* visen_train_mode, Ptr<SIFT> extractor, const vector<string> files) {
    size_t in_len;
    uint8_t* in;

    // adding
    for (unsigned i = 0; i < files.size(); i++) {
        if(i % 100 == 0)
            printf("Train img (%u/%lu) %s\n", i, files.size(), files[i].c_str());

        add_train_images(&in, &in_len, extractor, files[i]);
        iee_comm(&ssl, in, in_len);
        free(in);
    }

    // train // this was for lsh vectors generation in enclave
    train(&in, &in_len);
    iee_comm(&ssl, in, in_len);
    free(in);
}
/*
void visen_train_client_lsh(mbedtls_ssl_context ssl, size_t desc_len, unsigned visen_nr_clusters, char* visen_train_mode, Ptr<SIFT> extractor, const vector<string> files) {
    size_t in_len;
    uint8_t* in;

    vector<float*> gaussians(visen_nr_clusters);
    std::default_random_engine generator(time(0));
    std::normal_distribution<float> distribution(0.0, 25.0);

    for (unsigned i = 0; i < visen_nr_clusters; ++i) {
        float* vec = (float*)malloc(desc_len * sizeof(float));
        for (unsigned j = 0; j < desc_len; ++j)
            vec[j] = distribution(generator);

        gaussians[i] = vec;
    }

    in[0] = OP_IEE_SET_CODEBOOK_CLIENT_LSH;

    void* p = in + sizeof(uint8_t);
    for (unsigned k = 0; k < visen_nr_clusters; ++k) {
        memcpy(p, gaussians[k], desc_len * sizeof(float));
        p = (unsigned char*)p + desc_len * sizeof(float);
    }

    iee_comm(&ssl, in, in_len);
    free(in);
}*/

void visen_add_files(mbedtls_ssl_context ssl, Ptr<SIFT> extractor, const vector<string> files) {
    size_t in_len;
    uint8_t* in;

    for (unsigned i = 0; i < files.size(); i++) {
        if(i % 1000 == 0)
            printf("Add img (%u/%lu)\n", i, files.size());
        add_images(&in, &in_len, extractor, files[i]);
        iee_comm(&ssl, in, in_len);
        free(in);
    }
}

int main(int argc, char** argv) {
    // static params
    const char* server_name = IEE_HOSTNAME;
    const int server_port = IEE_PORT;
    const size_t desc_len = 128;

    config_t cfg;
    config_init(&cfg);

    if(!config_read_file(&cfg, "../isen.cfg")) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        exit(1);
    }

    int use_text = 0;
    config_lookup_int(&cfg, "use_text", &use_text);

    int use_images = 0;
    config_lookup_int(&cfg, "use_images", &use_images);

    unsigned bisen_nr_docs;
    config_lookup_int(&cfg, "bisen.nr_docs", (int*)&bisen_nr_docs);

    char* bisen_doc_type;
    config_lookup_string(&cfg, "bisen.doc_type", (const char**)&bisen_doc_type);

    char* visen_train_mode, *visen_train_technique, *visen_search_mode, *visen_clusters_file;
    unsigned visen_descriptor_threshold, visen_nr_clusters;
    config_lookup_string(&cfg, "visen.train_technique", (const char**)&visen_train_technique);
    config_lookup_string(&cfg, "visen.train_mode", (const char**)&visen_train_mode);
    config_lookup_string(&cfg, "visen.search_mode", (const char**)&visen_search_mode);
    config_lookup_string(&cfg, "visen.clusters_file", (const char**)&visen_clusters_file);
    config_lookup_int(&cfg, "visen.descriptor_threshold", (int*)&visen_descriptor_threshold);
    config_lookup_int(&cfg, "visen.nr_clusters", (int*)&visen_nr_clusters);


    vector<string> queries;
    config_setting_t* queries_setting = config_lookup(&cfg, "bisen.queries");
    const int count = config_setting_length(queries_setting);

    for(int i = 0; i < count; ++i) {
        config_setting_t* q = config_setting_get_elem(queries_setting, i);
        queries.push_back(string(config_setting_get_string(q)));
    }

    // parse terminal arguments
    int c;
    while ((c = getopt(argc, argv, "hk:b:")) != -1) {
        switch (c) {
            case 'k':
                visen_nr_clusters = std::stoul(optarg);
                break;
            case 'b':
                bisen_nr_docs = std::stoi(optarg);
                break;
            case 'h':
                printf("Usage: ./Client nr-imgs\n");
                exit(0);
            case '?':
                if (optopt == 'c')
                    fprintf(stderr, "-%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                exit(1);
            default:
                exit(-1);
        }
    }

    const int nr_images = -1;

    if(use_text)
        printf("BISEN: %d documents\n", bisen_nr_docs);
    else
        printf("BISEN: disabled\n");

    if(use_images)
        printf("VISEN: train %s; search %s\n", visen_train_mode, visen_search_mode);
    else
        printf("VISEN: disabled\n");


    // init mbedtls
    // put port in char buf
    char port[5];
    sprintf(port, "%d", server_port);

    int ret;
    mbedtls_net_context server_fd;
    uint32_t flags;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;

    // initialise the RNG and the session data
    mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    printf("seeding the random number generator...\n");
    mbedtls_entropy_init(&entropy);
    const char* pers = "ssl_client1";
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)pers,
                                     strlen(pers))) != 0) {
        printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        exit(1);
    }

    // initialise certificates
    printf("loading the CA root certificate...\n");
    ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char*)mbedtls_test_cas_pem, mbedtls_test_cas_pem_len);
    if (ret < 0) {
        printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        exit(1);
    }

    printf("ok (%d skipped)\n", ret);

    // start connection
    printf("connecting to tcp/%s/%s...\n", server_name, port);
    if ((ret = mbedtls_net_connect(&server_fd, server_name, port, MBEDTLS_NET_PROTO_TCP)) != 0) {
        printf(" failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
        exit(1);
    }

    printf("setting up the SSL/TLS structure...\n");
    if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
        exit(1);
    }

    /* OPTIONAL is not optimal for security,
     * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
        printf("failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
        exit(1);
    }

    if ((ret = mbedtls_ssl_set_hostname(&ssl, server_name)) != 0) {
        printf("failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
        exit(1);
    }

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    printf("performing the SSL/TLS handshake...\n");
    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            printf("failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
            exit(1);
        }
    }

    // verify the server certificate
    printf("verifying peer X.509 certificate...\n");

    // in real life, we probably want to bail out when ret != 0
    if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0) {
        printf("failed\n");

        char vrfy_buf[512];
        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "! ", flags);
        printf("%s\n", vrfy_buf);
    }

    struct timeval start, end;

    //////////// BISEN ////////////
    if(use_text) {
        SseClient client;

        gettimeofday(&start, NULL);
        bisen_setup(ssl, &client);
        gettimeofday(&end, NULL);
        printf("-- BISEN setup: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

        gettimeofday(&start, NULL);
        bisen_update(ssl, &client, bisen_nr_docs, bisen_doc_type);
        gettimeofday(&end, NULL);
        printf("-- BISEN updates: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

        gettimeofday(&start, NULL);
        bisen_search(ssl, &client, queries);
        gettimeofday(&end, NULL);
        printf("-- BISEN searches: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));
    }

    ///////////////////////////////
    if(use_images) {
        // image descriptor parameters
        Ptr<SIFT> extractor = SIFT::create(visen_descriptor_threshold);
        const vector<string> files = get_filenames(nr_images, "/home/guilherme/Datasets/inria");

        // init iee and server
        gettimeofday(&start, NULL);
        visen_setup(ssl, desc_len, visen_nr_clusters);
        gettimeofday(&end, NULL);
        printf("-- VISEN setup: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

        // train
        gettimeofday(&start, NULL);
        if(!strcmp(visen_train_technique, "client_kmeans")) {
            visen_train_client_kmeans(ssl, desc_len, visen_nr_clusters, visen_train_mode, visen_clusters_file, extractor);
        } else if(!strcmp(visen_train_technique, "iee_kmeans")) {
            visen_train_iee_kmeans(ssl, visen_train_mode, extractor, files);
        }

        gettimeofday(&end, NULL);
        printf("-- VISEN train: %ldms %s--\n", untrusted_util::time_elapsed_ms(start, end), visen_train_technique);

        // add images to repository
        gettimeofday(&start, NULL);
        visen_add_files(ssl, extractor, files);
        gettimeofday(&end, NULL);
        printf("-- VISEN add imgs: %ldms %lu imgs--\n", untrusted_util::time_elapsed_ms(start, end), files.size());

        // search
        gettimeofday(&start, NULL);
        search_test(&ssl, extractor);
        gettimeofday(&end, NULL);
        printf("-- VISEN searches: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

        // clear
        size_t in_len;
        uint8_t* in;
        clear(&in, &in_len);
        iee_comm(&ssl, in, in_len);
        free(in);
    }

    // close ssl connection
    mbedtls_ssl_close_notify(&ssl);

    // ssl cleanup
    mbedtls_net_free(&server_fd);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return 0;
}

    /*} else {
train_load_clusters(&in, &in_len);
iee_comm(&ssl, in, in_len);
free(in);
}*/

/*
if (load_uee) {
    // tell iee to load images from disc
    unsigned char op = OP_IEE_READ_MAP;
    iee_comm(&ssl, &op, 1);
} else {*/
#if CLUSTERING == C_LSH
add_images_lsh(&in, &in_len, surf, files[i]);
#endif

/*if (write_uee) {
    // send persist message to iee
    unsigned char op = OP_IEE_WRITE_MAP;
    iee_comm(&ssl, &op, 1);
}*/

/*
//TODO perform a search from args?
    gettimeofday(&start, NULL);

    search(&in, &in_len, surf, get_filenames(10, "/home/guilherme/Datasets/inria")[0]);
    iee_send(&ssl, in, in_len);
    free(in);

    // receive response and print
    size_t res_len;
    uint8_t* res;
    iee_recv(&ssl, &res, &res_len);

    unsigned nr;
    memcpy(&nr, res, sizeof(unsigned));
    printf("nr of imgs: %u\n", nr);

    for (unsigned j = 0; j < nr; ++j) {
        unsigned long id;
        memcpy(&id, res + sizeof(size_t) + j * SRC_RES_LEN, sizeof(unsigned long));

        double score;
        memcpy(&score, res + sizeof(size_t) + j * SRC_RES_LEN + sizeof(unsigned long), sizeof(double));

        printf("%lu %f\n", id, score);

    }

    gettimeofday(&end, NULL);
    printf("-- Search elapsed time: %ldms--\n", untrusted_util::time_elapsed_ms(start, end));
 */
