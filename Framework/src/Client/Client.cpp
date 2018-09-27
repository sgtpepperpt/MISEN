#include "Client.h"

#include "untrusted_util.h"
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

#include "ImageSearch.h"
#include "visen_tests.h"
#include "bisen_tests.h"
#include "misen_tests.h"
#include "util.h"

using namespace std;


typedef struct configs {
    int use_text = 0, use_images = 0, use_multimodal = 0;

    unsigned bisen_nr_docs = 1000;
    char* bisen_doc_type, *bisen_dataset_dir;
    vector<string> bisen_queries;

    unsigned visen_nr_docs = 0;
    char* visen_train_mode, *visen_train_technique, *visen_add_mode, *visen_search_mode, *visen_clusters_file, *visen_dataset_dir;
    unsigned visen_descriptor_threshold, visen_nr_clusters, visen_desc_len;

    unsigned misen_nr_docs = 1000, misen_nr_queries = 10;
    char* misen_text_dir, *misen_img_dir;
} configs;

void separated_tests(const configs* const settings, secure_connection* conn) {
    struct timeval start, end;

    //////////// BISEN ////////////
    if(settings->use_text) {
        SseClient client;
        vector<string> txt_paths = list_txt_files(-1, settings->bisen_dataset_dir); // get all paths and decide nr of docs in update (wiki mode is special)
        sort(txt_paths.begin(), txt_paths.end(), greater<string>()); // documents with more articles happen to be at the end for the wikipedia dataset

        gettimeofday(&start, NULL);
        bisen_setup(conn, &client);
        gettimeofday(&end, NULL);
        printf("-- BISEN setup: %lfms --\n", untrusted_util::time_elapsed_ms(start, end));

        gettimeofday(&start, NULL);
        bisen_update(conn, &client, settings->bisen_doc_type, settings->bisen_nr_docs, txt_paths);
        gettimeofday(&end, NULL);
        printf("-- BISEN TOTAL updates: %lf ms %d docs --\n", untrusted_util::time_elapsed_ms(start, end), settings->bisen_nr_docs);

        bisen_search(conn, &client, settings->bisen_queries);
    }

    ///////////////////////////////
    if(settings->use_images) {
        // image descriptor parameters
        descriptor_t descriptor = create_descriptor(settings->visen_descriptor_threshold);
        const vector<string> files = list_img_files(!settings->visen_nr_docs? -1 : settings->visen_nr_docs, settings->visen_dataset_dir);

        // init iee and server
        gettimeofday(&start, NULL);
        visen_setup(conn, settings->visen_desc_len, settings->visen_nr_clusters, settings->visen_train_technique);
        gettimeofday(&end, NULL);
        printf("-- VISEN setup: %lfms --\n", untrusted_util::time_elapsed_ms(start, end));

        // train
        gettimeofday(&start, NULL);
        if(!strcmp(settings->visen_train_technique, "client_kmeans"))
            visen_train_client_kmeans(conn, settings->visen_desc_len, settings->visen_nr_clusters, settings->visen_train_mode, settings->visen_clusters_file, settings->visen_dataset_dir, descriptor);
        else if(!strcmp(settings->visen_train_technique, "iee_kmeans"))
            visen_train_iee_kmeans(conn, settings->visen_train_mode, descriptor, files);
        else if(!strcmp(settings->visen_train_technique, "lsh"))
            visen_train_client_lsh(conn, settings->visen_train_mode, settings->visen_desc_len, settings->visen_nr_clusters);

        gettimeofday(&end, NULL);
        printf("-- VISEN train: %lfms %s %s--\n", untrusted_util::time_elapsed_ms(start, end), settings->visen_train_technique, settings->visen_train_mode);

        // add images to repository
        if(!strcmp(settings->visen_add_mode, "normal")) {
            visen_add_files(conn, descriptor, files);
        } else if(!strcmp(settings->visen_add_mode, "load")) {
            // tell iee to load images from disc
            unsigned char op = OP_IEE_READ_MAP;
            iee_comm(conn, &op, 1);
        } else {
            printf("Add mode error\n");
            exit(1);
        }

        if(!strcmp(settings->visen_add_mode, "normal")) {
            // send persist storage message to iee, useful for debugging and testing
            unsigned char op = OP_IEE_WRITE_MAP;
            iee_comm(conn, &op, 1);
        }

        // search
        const int dbg_limit = -1;
        search_test(conn, descriptor, dbg_limit);

        // dump benchmark results
        size_t in_len;
        uint8_t* in;
        dump_bench(&in, &in_len);
        iee_comm(conn, in, in_len);
        free(in);

        // clear
        clear(&in, &in_len);
        iee_comm(conn, in, in_len);
        free(in);
    }
}

void multimodal_tests(const configs* const settings, secure_connection* conn) {
    struct timeval start, end;

    // image descriptor parameters
    descriptor_t descriptor = create_descriptor(settings->visen_descriptor_threshold);
    SseClient client;

    vector<string> txt_paths = list_txt_files(settings->misen_nr_docs, settings->misen_text_dir);
    vector<string> img_paths = list_img_files(settings->misen_nr_docs, settings->misen_img_dir);

    // have txt in the same order as imgs
    sort(txt_paths.begin(), txt_paths.end());
    sort(img_paths.begin(), img_paths.end());

    // init
    gettimeofday(&start, NULL);
    visen_setup(conn, settings->visen_desc_len, settings->visen_nr_clusters, settings->visen_train_technique);
    bisen_setup(conn, &client);
    gettimeofday(&end, NULL);
    printf("-- MISEN setup: %lfms --\n", untrusted_util::time_elapsed_ms(start, end));

    // train (for images)
    gettimeofday(&start, NULL);
    visen_train_client_kmeans(conn, settings->visen_desc_len, settings->visen_nr_clusters, (char*)"load", settings->visen_clusters_file, NULL, descriptor);
    gettimeofday(&end, NULL);
    printf("-- MISEN train: %lfms %s--\n", untrusted_util::time_elapsed_ms(start, end), settings->visen_train_technique);

    // add documents to iee
    gettimeofday(&start, NULL);
    bisen_update(conn, &client, (char*)"normal", settings->misen_nr_docs, txt_paths);
    visen_add_files(conn, descriptor, img_paths);
    gettimeofday(&end, NULL);
    printf("-- MISEN updates: %lfms --\n", untrusted_util::time_elapsed_ms(start, end));

    // searches
    gettimeofday(&start, NULL);
    vector<pair<string, string>> multimodal_queries = generate_multimodal_queries(txt_paths, img_paths, settings->misen_nr_queries);
    misen_search(conn, &client, descriptor, multimodal_queries);
    gettimeofday(&end, NULL);
    printf("-- MISEN searches: %lfms %lu queries --\n", untrusted_util::time_elapsed_ms(start, end), multimodal_queries.size());

    // dump benchmark results
    size_t in_len;
    uint8_t* in;
    dump_bench(&in, &in_len);
    iee_comm(conn, in, in_len);
    free(in);

    // cleanup
    clear(&in, &in_len);
    iee_comm(conn, in, in_len);
    free(in);
}

int main(int argc, char** argv) {
    configs program_configs;

    // static params
    const char* server_name = IEE_HOSTNAME;
    const int server_port = IEE_PORT;

#if DESCRIPTOR == DESC_SIFT
    program_configs.visen_desc_len = 128;
#else
    program_configs.visen_desc_len = 64;
#endif

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

    config_lookup_int(&cfg, "visen.nr_docs", (int*)&program_configs.visen_nr_docs);
    config_lookup_string(&cfg, "visen.train_technique", (const char**)&program_configs.visen_train_technique);
    config_lookup_string(&cfg, "visen.train_mode", (const char**)&program_configs.visen_train_mode);
    config_lookup_string(&cfg, "visen.add_mode", (const char**)&program_configs.visen_add_mode);
    config_lookup_string(&cfg, "visen.search_mode", (const char**)&program_configs.visen_search_mode);
    config_lookup_string(&cfg, "visen.clusters_file", (const char**)&program_configs.visen_clusters_file);
    config_lookup_string(&cfg, "visen.dataset_dir", (const char**)&program_configs.visen_dataset_dir);
    config_lookup_int(&cfg, "visen.descriptor_threshold", (int*)&program_configs.visen_descriptor_threshold);
    config_lookup_int(&cfg, "visen.nr_clusters", (int*)&program_configs.visen_nr_clusters);

    config_lookup_string(&cfg, "misen.text_dir", (const char**)&program_configs.misen_text_dir);
    config_lookup_string(&cfg, "misen.img_dir", (const char**)&program_configs.misen_img_dir);
    config_lookup_int(&cfg, "misen.nr_docs", (int*)&program_configs.misen_nr_docs);
    config_lookup_int(&cfg, "misen.nr_queries", (int*)&program_configs.misen_nr_queries);

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
                program_configs.visen_nr_docs = program_configs.bisen_nr_docs;
                break;
            case 'h':
                printf("Usage: ./Client [-b nr_docs] [-k nr_clusters]\n");
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
    secure_connection* conn;
    untrusted_util::init_secure_connection(&conn, server_name, server_port);

    if(program_configs.use_multimodal) {
        multimodal_tests(&program_configs, conn);
    } else {
        // do bisen and visen test separately
        separated_tests(&program_configs, conn);
    }

    // close ssl connection
    untrusted_util::close_secure_connection(conn);

    return 0;
}

/*} else {
train_load_clusters(&in, &in_len);
iee_comm(&ssl, in, in_len);
free(in);
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
