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
#include "visen_tests.h"
#include "bisen_tests.h"

using namespace cv;
using namespace cv::xfeatures2d;
using namespace std;

static void my_debug(void* ctx, int level, const char* file, int line, const char* str) {
    ((void)level);
    fprintf((FILE*)ctx, "%s:%04d: %s", file, line, str);
    fflush((FILE*)ctx);
}

typedef struct configs {
    int use_text = 0, use_images = 0, use_multimodal = 0;

    unsigned bisen_nr_docs = 1000;
    char* bisen_doc_type, *bisen_dataset_dir;
    vector<string> bisen_queries;

    char* visen_train_mode, *visen_train_technique, *visen_add_mode, *visen_search_mode, *visen_clusters_file, *visen_dataset_dir;
    unsigned visen_descriptor_threshold, visen_nr_clusters, visen_desc_len;
} configs;

void separated_tests(const configs* const settings, mbedtls_ssl_context* ssl) {
    struct timeval start, end;

    //////////// BISEN ////////////
    if(settings->use_text) {
        SseClient client;

        gettimeofday(&start, NULL);
        bisen_setup(ssl, &client);
        gettimeofday(&end, NULL);
        printf("-- BISEN setup: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

        gettimeofday(&start, NULL);
        bisen_update(ssl, &client, settings->bisen_nr_docs, settings->bisen_doc_type, settings->bisen_dataset_dir);
        gettimeofday(&end, NULL);
        printf("-- BISEN updates: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

        gettimeofday(&start, NULL);
        bisen_search(ssl, &client, settings->bisen_queries);
        gettimeofday(&end, NULL);
        printf("-- BISEN searches: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));
    }

    ///////////////////////////////
    if(settings->use_images) {
        // image descriptor parameters
        Ptr<SIFT> extractor = SIFT::create(settings->visen_descriptor_threshold);
        const vector<string> files = get_filenames(-1, "/home/guilherme/Datasets/inria");

        // init iee and server
        gettimeofday(&start, NULL);
        visen_setup(ssl, settings->visen_desc_len, settings->visen_nr_clusters);
        gettimeofday(&end, NULL);
        printf("-- VISEN setup: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

        // train
        gettimeofday(&start, NULL);
        if(!strcmp(settings->visen_train_technique, "client_kmeans"))
            visen_train_client_kmeans(ssl, settings->visen_desc_len, settings->visen_nr_clusters, settings->visen_train_mode, settings->visen_clusters_file, settings->visen_dataset_dir, extractor);
        else if(!strcmp(settings->visen_train_technique, "iee_kmeans"))
            visen_train_iee_kmeans(ssl, settings->visen_train_mode, extractor, files);
        gettimeofday(&end, NULL);
        printf("-- VISEN train: %ldms %s--\n", untrusted_util::time_elapsed_ms(start, end), settings->visen_train_technique);

        // add images to repository
        gettimeofday(&start, NULL);
        if(!strcmp(settings->visen_add_mode, "normal")) {
            visen_add_files(ssl, extractor, files);
        } else if(!strcmp(settings->visen_add_mode, "load")) {
            // tell iee to load images from disc
            unsigned char op = OP_IEE_READ_MAP;
            iee_comm(ssl, &op, 1);
        } else {
            printf("Add mode error\n");
            exit(1);
        }
        gettimeofday(&end, NULL);
        printf("-- VISEN add imgs: %ldms %lu imgs--\n", untrusted_util::time_elapsed_ms(start, end), files.size());

        if(!strcmp(settings->visen_add_mode, "normal")) {
            // send persist storage message to iee, useful for debugging and testing
            unsigned char op = OP_IEE_WRITE_MAP;
            iee_comm(ssl, &op, 1);
        }

        // search
        gettimeofday(&start, NULL);
        search_test(ssl, extractor);
        gettimeofday(&end, NULL);
        printf("-- VISEN searches: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

        // clear
        size_t in_len;
        uint8_t* in;
        clear(&in, &in_len);
        iee_comm(ssl, in, in_len);
        free(in);
    }
}

void misen_search(mbedtls_ssl_context* ssl, SseClient* client, Ptr<SIFT> extractor, vector<pair<string, string>> queries) {
    for (unsigned k = 0; k < queries.size(); k++) {
        string text_query = queries[k].first;
        string img_file_path = queries[k].second;

        printf("\n----------------------------\n");
        printf("Query %d: %s %s\n", k, text_query.c_str(), img_file_path.c_str());

        size_t query_bisen_len, query_visen_len;
        uint8_t* query_bisen, *query_visen;

        query_bisen_len = client->search(text_query, &query_bisen);
        search(&query_visen, &query_visen_len, extractor, img_file_path);

        uint8_t multimodal_query[2 * sizeof(size_t) + query_bisen_len + query_visen_len];
        memcpy(multimodal_query, &query_bisen_len, sizeof(size_t));
        memcpy(multimodal_query + sizeof(size_t), query_bisen, query_bisen_len);

        memcpy(multimodal_query + sizeof(size_t) + query_bisen_len, &query_visen_len, sizeof(size_t));
        memcpy(multimodal_query + 2 * sizeof(size_t) + query_bisen_len, query_visen, query_visen_len);

        free(query_bisen);
        free(query_visen);

        iee_send(ssl, multimodal_query, 2 * sizeof(size_t) + query_bisen_len + query_visen_len);

        uint8_t* res;
        size_t res_len;
        iee_recv(ssl, &res, &res_len);

        unsigned nr_docs;
        memcpy(&nr_docs, res, sizeof(int));
        printf("Number of docs: %d\n", nr_docs);

        for (unsigned i = 0; i < nr_docs; ++i) {
            unsigned d;
            double s;
            memcpy(&d, res + sizeof(unsigned) + i * (sizeof(unsigned) + sizeof(double)), sizeof(unsigned));
            memcpy(&s, res + sizeof(unsigned) + i * (sizeof(unsigned) + sizeof(double)) + sizeof(unsigned), sizeof(double));
            printf("%ul %f\n", d, s);
        }

        free(res);
    }
}

vector<pair<string, string>> generate_multimodal_queries(unsigned nr_docs) {
    vector<pair<string, string>> multimodal_queries;

    vector<string> txt_paths;
    listTxtFiles("/home/guilherme/Datasets/mirflickr/meta/tags/", txt_paths);
    sort(txt_paths.begin(), txt_paths.end(), greater<string>()); // have txt in the same order as imgs

    vector<string> img_paths = get_filenames(nr_docs, "/home/guilherme/Datasets/mirflickr/");

    for (unsigned i = 0; i < nr_docs; ++i) {
        multimodal_queries.push_back(make_pair(txt_paths[i], img_paths[i]));
        cout << txt_paths[i] << " " << img_paths[i] << endl;
    }

    return multimodal_queries;
}

void multimodal_tests(const configs* const settings, mbedtls_ssl_context* ssl) {
    struct timeval start, end;

    // image descriptor parameters
    Ptr<SIFT> extractor = SIFT::create(settings->visen_descriptor_threshold);
    const vector<string> files = get_filenames(-1, "/home/guilherme/Datasets/inria");
    SseClient client;

    // init
    gettimeofday(&start, NULL);
    visen_setup(ssl, settings->visen_desc_len, settings->visen_nr_clusters);

    bisen_setup(ssl, &client);
    gettimeofday(&end, NULL);
    printf("-- MISEN setup: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

    // train (for images)
    gettimeofday(&start, NULL);
    if(!strcmp(settings->visen_train_technique, "client_kmeans")) {
        visen_train_client_kmeans(ssl, settings->visen_desc_len, settings->visen_nr_clusters, settings->visen_train_mode, settings->visen_clusters_file, settings->visen_dataset_dir, extractor);
    } else if(!strcmp(settings->visen_train_technique, "iee_kmeans")) {
        visen_train_iee_kmeans(ssl, settings->visen_train_mode, extractor, files);
    }

    gettimeofday(&end, NULL);
    printf("-- MISEN train: %ldms %s--\n", untrusted_util::time_elapsed_ms(start, end), settings->visen_train_technique);

    // add documents to iee
    gettimeofday(&start, NULL);
    bisen_update(ssl, &client, settings->bisen_nr_docs, settings->bisen_doc_type, settings->bisen_dataset_dir);
    visen_add_files(ssl, extractor, files);
    gettimeofday(&end, NULL);
    printf("-- MISEN updates: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

    // searches
    gettimeofday(&start, NULL);
    vector<pair<string, string>> multimodal_queries = generate_multimodal_queries(settings->bisen_nr_docs);
    misen_search(ssl, &client, extractor, multimodal_queries);
    gettimeofday(&end, NULL);
    printf("-- MISEN searches: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

    // cleanup
    size_t in_len;
    uint8_t* in;
    clear(&in, &in_len);
    iee_comm(ssl, in, in_len);
    free(in);
}

int main(int argc, char** argv) {
    configs program_configs;

    // static params
    const char* server_name = IEE_HOSTNAME;
    const int server_port = IEE_PORT;
    program_configs.visen_desc_len = 128;

    config_t cfg;
    config_init(&cfg);

    if(!config_read_file(&cfg, "../isen.cfg")) {
        fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        exit(1);
    }

    config_lookup_int(&cfg, "use_text", &program_configs.use_text);
    config_lookup_int(&cfg, "use_images", &program_configs.use_images);
    config_lookup_int(&cfg, "use_multimodal", &program_configs.use_multimodal);

    config_lookup_int(&cfg, "bisen.nr_docs", (int*)&program_configs.bisen_nr_docs);
    config_lookup_string(&cfg, "bisen.doc_type", (const char**)&program_configs.bisen_doc_type);
    config_lookup_string(&cfg, "bisen.dataset_dir", (const char**)&program_configs.bisen_dataset_dir);

    config_lookup_string(&cfg, "visen.train_technique", (const char**)&program_configs.visen_train_technique);
    config_lookup_string(&cfg, "visen.train_mode", (const char**)&program_configs.visen_train_mode);
    config_lookup_string(&cfg, "visen.add_mode", (const char**)&program_configs.visen_add_mode);
    config_lookup_string(&cfg, "visen.search_mode", (const char**)&program_configs.visen_search_mode);
    config_lookup_string(&cfg, "visen.clusters_file", (const char**)&program_configs.visen_clusters_file);
    config_lookup_string(&cfg, "visen.dataset_dir", (const char**)&program_configs.visen_dataset_dir);
    config_lookup_int(&cfg, "visen.descriptor_threshold", (int*)&program_configs.visen_descriptor_threshold);
    config_lookup_int(&cfg, "visen.nr_clusters", (int*)&program_configs.visen_nr_clusters);

    config_setting_t* queries_setting = config_lookup(&cfg, "bisen.queries");
    const int count = config_setting_length(queries_setting);

    for(int i = 0; i < count; ++i) {
        config_setting_t* q = config_setting_get_elem(queries_setting, i);
        program_configs.bisen_queries.push_back(string(config_setting_get_string(q)));
    }

    // parse terminal arguments
    int c;
    while ((c = getopt(argc, argv, "hk:b:")) != -1) {
        switch (c) {
            case 'k':
                program_configs.visen_nr_clusters = (unsigned)std::stoi(optarg);
                break;
            case 'b':
                program_configs.bisen_nr_docs = (unsigned)std::stoi(optarg);
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

    if(program_configs.use_multimodal) {
        printf("MISEN: %d documents\n", program_configs.bisen_nr_docs);
    } else {
        if(program_configs.use_text)
            printf("BISEN: %d documents\n", program_configs.bisen_nr_docs);
        else
            printf("BISEN: disabled\n");

        if(program_configs.use_images)
            printf("VISEN: train %s; search %s\n", program_configs.visen_train_mode, program_configs.visen_search_mode);
        else
            printf("VISEN: disabled\n");
    }

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

    if(program_configs.use_multimodal) {
        multimodal_tests(&program_configs, &ssl);
    } else {
        // do bisen and visen test separately
        separated_tests(&program_configs, &ssl);
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

#if CLUSTERING == C_LSH
add_images_lsh(&in, &in_len, surf, files[i]);
#endif

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
