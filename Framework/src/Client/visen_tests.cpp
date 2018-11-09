#include "visen_tests.h"

#include "definitions.h"

#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <random>

#include <opencv2/opencv.hpp>

#include "ImageSearch.h"
#include "util.h"

extern "C" {
#include "rbisen/Utils.h"
#include "rbisen/types.h"
}

using namespace std;

float* client_train(const char* path, const unsigned nr_clusters, const unsigned desc_len, feature_extractor desc) {
    const vector<string> imgs = list_img_files(5000, path);

    //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce");
    //BOWImgDescriptorExtractor* bowExtractor = new BOWImgDescriptorExtractor(surf, matcher);

    cv::TermCriteria terminate_criterion;
    terminate_criterion.epsilon = FLT_EPSILON;
    cv::BOWKMeansTrainer bowTrainer(nr_clusters, terminate_criterion, 3, cv::KMEANS_PP_CENTERS);

    for (const string p : imgs) {
#if TRAIN_MAIN_ONLY
        const char* str = p.c_str();
        const size_t len = strlen(str);
        if(!(str[len-5] == '0' && str[len-6] == '0'))
            continue;
#endif

        //printf("train %s\n", p.c_str());
        cv::Mat image = cv::imread(p);
        cv::Mat descriptors = desc.get_features(image);

        //cout << "nr_desc " << descriptors.size().height << endl;

        if(descriptors.size().height)
            bowTrainer.add(descriptors);
    }

    printf("Built codebook with %d descriptors (avg. %lu)\n", bowTrainer.descriptorsCount(), bowTrainer.descriptorsCount()/imgs.size());
    cv::Mat codebook = bowTrainer.cluster();
    //bowExtractor->setVocabulary(codebook);
    //bowExtractor->compute()

    //const size_t desc_len = (size_t)codebook.size().width;
    const size_t nr_desc = (size_t)codebook.size().height;

    cout << "desc_len " << desc_len << endl;
    cout << "nr_desc " << nr_desc << endl;

    float* centroids = (float*)malloc(nr_desc * desc_len * sizeof(float));
    for (unsigned i = 0; i < nr_desc; i++) {
        for (size_t j = 0; j < desc_len; j++)
            centroids[i * desc_len + j] = *codebook.ptr<float>(i, j);
    }

    codebook.release();
    return centroids;
}

void visen_setup(secure_connection* conn, size_t desc_len, unsigned visen_nr_clusters, const char* train_technique) {
    size_t in_len = 0;
    uint8_t* in = NULL;

    init(&in, &in_len, visen_nr_clusters, desc_len, train_technique);
    iee_comm(conn, in, in_len);
    free(in);
}

void visen_train_client_kmeans(secure_connection* conn, unsigned visen_nr_clusters, char* visen_train_mode, char* visen_centroids_file, const char* visen_dataset_dir, feature_extractor desc) {
    size_t in_len = sizeof(uint8_t) + visen_nr_clusters * desc.get_desc_len() * sizeof(float);
    uint8_t* in = (uint8_t*)malloc(sizeof(uint8_t) + visen_nr_clusters * desc.get_desc_len() * sizeof(float));
    in[0] = OP_IEE_SET_CODEBOOK_CLIENT_KMEANS;

    float* centroids;
    if(!strcmp(visen_train_mode, "train")) {
        centroids = client_train(visen_dataset_dir, visen_nr_clusters, desc.get_desc_len(), desc);

        FILE* file = fopen(visen_centroids_file, "wb");
        if (!file) {
            printf("Could not write centroids file\n");
            exit(1);
        }

        fwrite(centroids, visen_nr_clusters, desc.get_desc_len() * sizeof(float), file);
        fclose(file);
    } else if(!strcmp(visen_train_mode, "load")) {
        centroids = (float*)malloc(visen_nr_clusters * desc.get_desc_len() * sizeof(float));
        FILE* file = fopen(visen_centroids_file, "rb");
        if (!file) {
            printf("Could not read centroids file\n");
            exit(1);
        }

        fread(centroids, visen_nr_clusters, desc.get_desc_len() * sizeof(float), file);
        fclose(file);
    } else {
        printf("error in train mode\n");
        exit(1);
    }

    memcpy(in + 1, centroids, visen_nr_clusters * desc.get_desc_len() * sizeof(float));
    iee_comm(conn, in, in_len);
    free(in);
    free(centroids);
}

void visen_train_iee_kmeans(secure_connection* conn, char* visen_train_mode, feature_extractor desc, const vector<string> files) {
    size_t in_len;
    uint8_t* in;

    size_t total_desc = 0;

    // adding
    size_t counter = 0;
    for (const string p : files) {
        if(counter % 100 == 0)
            printf("Train img (%lu/%lu) %s\n", counter++, files.size(), p.c_str());

#if TRAIN_MAIN_ONLY
        const char* str = p.c_str();
        const size_t len = strlen(str);
        if(!(str[len-5] == '0' && str[len-6] == '0'))
            continue;
#endif

        total_desc += add_train_images(&in, &in_len, desc, p);
        iee_comm(conn, in, in_len);
        free(in);
    }

    // train // this was also for lsh vectors generation in enclave
    train(&in, &in_len);
    iee_comm(conn, in, in_len);
    free(in);

    printf("total descriptors for train: %lu\n", total_desc);
}

void visen_train_client_lsh(secure_connection* conn, char* visen_train_mode, size_t desc_len, unsigned visen_nr_clusters) {
    size_t in_len = sizeof(uint8_t) + visen_nr_clusters * desc_len * sizeof(float);
    uint8_t in[in_len];

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

    iee_comm(conn, in, in_len);
}

void visen_add_files(secure_connection* conn, feature_extractor desc, const vector<string> files) {
    struct timeval start, end;
    double total_client = 0, total_iee = 0;

    size_t in_len;
    uint8_t* in;

    for (unsigned i = 0; i < files.size(); i++) {
        if(i % 1000 == 0)
            printf("Add img (%u/%lu)\n", i, files.size());

        gettimeofday(&start, NULL);
        add_images(&in, &in_len, desc, files[i]);
        gettimeofday(&end, NULL);
        total_client += untrusted_util::time_elapsed_ms(start, end);

        gettimeofday(&start, NULL);
        iee_comm(conn, in, in_len);
        gettimeofday(&end, NULL);
        total_iee += untrusted_util::time_elapsed_ms(start, end);

        free(in);
    }

    printf("-- VISEN TOTAL add: %lf ms (%lu imgs) --\n", total_client + total_iee, files.size());
    printf("-- VISEN add client: %lf ms (of which %lf ms extract, %lf ms read) --\n", total_client, get_feature_time_add(), get_read_time_add());
    printf("-- VISEN add iee w/ net: %lf ms --\n", total_iee);
}
