#include "visen_tests.h"

#include "definitions.h"

#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <random>

#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>

#include "ImageSearch.h"

extern "C" {
#include "rbisen/Utils.h"
#include "rbisen/types.h"
}

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;

void extractHolidayFileNames(const char* base_path, int nImgs, std::map<int, std::string> &imgs) {
    DIR* dir;
    struct dirent* ent;
    int i = 0;
    if ((dir = opendir(base_path)) != NULL) {
        while ((ent = readdir(dir)) != NULL && i < nImgs) {
            std::string fname = ent->d_name;
            const size_t pos = fname.find(".jpg");
            if (pos != std::string::npos) {
                std::string idString = fname.substr(0, pos);
                const int id = atoi(idString.c_str());
                std::string path = base_path;
                path += fname;
                imgs[id] = path;
                i++;
            }
        }
        closedir(dir);
    } else {
        printf("Util::processHolidayDataset couldn't open dir");
        exit(1);
    }
}

float* client_train(const char* path, const unsigned nr_clusters, const unsigned desc_len, Ptr<SIFT> descriptor) {
    map<int, string> imgs;
    extractHolidayFileNames(path, 50000, imgs);

    //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce");
    //BOWImgDescriptorExtractor* bowExtractor = new BOWImgDescriptorExtractor(surf, matcher);

    //timespec start = getTime();                         //getTime
    TermCriteria terminate_criterion;
    terminate_criterion.epsilon = FLT_EPSILON;
    BOWKMeansTrainer bowTrainer(nr_clusters, terminate_criterion, 3, KMEANS_PP_CENTERS);

    //std::srand(std::time(nullptr)); // use current time as seed for random generator
    //int random_variable = std::rand();
    for (map<int, string>::iterator it = imgs.begin(); it != imgs.end(); ++it) {
        const char* str = it->second.c_str();
        //const size_t len = strlen(str);

        /*if(!(str[len-5] == '0' && str[len-6] == '0'))
            continue;*/

        printf("train %s\n", str);
        Mat image = imread(it->second);

        vector<KeyPoint> keypoints;
        descriptor->detect(image, keypoints);

        Mat descriptors;
        descriptor->compute(image, keypoints, descriptors);

        bowTrainer.add(descriptors);
    }
    printf("build codebook with %d descriptors!\n", bowTrainer.descriptorsCount());
    Mat codebook = bowTrainer.cluster();
    //bowExtractor->setVocabulary(codebook);
    //bowExtractor->compute()
    //trainTime += diffSec(start, getTime());         //getTime
    //printf("Training Time: %f\n",trainTime);

    //const size_t desc_len = (size_t)codebook.size().width;
    const size_t nr_desc = (size_t)codebook.size().height;

    cout << "desc_len " << desc_len << endl;
    cout << "nr_desc " << nr_desc << endl;

    float* centroids = (float*)malloc(nr_desc * desc_len * sizeof(float));
    for (unsigned i = 0; i < nr_desc; i++) {
        for (size_t j = 0; j < desc_len; j++)
            centroids[i * desc_len + j] = *codebook.ptr<float>(i, j);
    }

    return centroids;
}


void visen_setup(mbedtls_ssl_context* ssl, size_t desc_len, unsigned visen_nr_clusters) {
    size_t in_len = 0;
    uint8_t* in = NULL;

    init(&in, &in_len, visen_nr_clusters, desc_len);
    iee_comm(ssl, in, in_len);
    free(in);
}

void visen_train_client_kmeans(mbedtls_ssl_context* ssl, size_t desc_len, unsigned visen_nr_clusters, char* visen_train_mode, char* visen_centroids_file, const char* visen_dataset_dir, Ptr<SIFT> extractor) {
    size_t in_len = sizeof(uint8_t) + visen_nr_clusters * desc_len * sizeof(float);
    uint8_t* in = (uint8_t*)malloc(sizeof(uint8_t) + visen_nr_clusters * desc_len * sizeof(float));
    in[0] = OP_IEE_SET_CODEBOOK_CLIENT_KMEANS;

    float* centroids;
    if(!strcmp(visen_train_mode, "train")) {
        centroids = client_train(visen_dataset_dir, visen_nr_clusters, desc_len, extractor);

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
    iee_comm(ssl, in, in_len);
    free(in);
    free(centroids);
}

void visen_train_iee_kmeans(mbedtls_ssl_context* ssl, char* visen_train_mode, Ptr<SIFT> extractor, const vector<string> files) {
    size_t in_len;
    uint8_t* in;

    // adding
    for (unsigned i = 0; i < files.size(); i++) {
        if(i % 100 == 0)
            printf("Train img (%u/%lu) %s\n", i, files.size(), files[i].c_str());

        add_train_images(&in, &in_len, extractor, files[i]);
        iee_comm(ssl, in, in_len);
        free(in);
    }

    // train // this was also for lsh vectors generation in enclave
    train(&in, &in_len);
    iee_comm(ssl, in, in_len);
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

void visen_add_files(mbedtls_ssl_context* ssl, Ptr<SIFT> extractor, const vector<string> files) {
    size_t in_len;
    uint8_t* in;

    for (unsigned i = 0; i < files.size(); i++) {
        if(i % 100 == 0)
            printf("Add img (%u/%lu)\n", i, files.size());
        add_images(&in, &in_len, extractor, files[i]);
        iee_comm(ssl, in, in_len);
        free(in);
    }
}
