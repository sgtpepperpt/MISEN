#include "ImageSearch.h"

#include "definitions.h"

#include <stdlib.h>
#include <dirent.h>
#include <map>
#include <fstream>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include "util.h"

using namespace cv;
using namespace std;

descriptor_t create_descriptor(int parameter) {
#if DESCRIPTOR == DESC_SIFT
    return cv::xfeatures2d::SIFT::create(parameter);
#else
    return cv::xfeatures2d::SURF::create(parameter);
#endif
}

#if STORE_RESULTS
void printHolidayResults(const char* path, std::map<unsigned long, std::vector<unsigned long>> results) {
    std::ofstream ofs(path);
    std::map<unsigned long, std::vector<unsigned long>>::iterator it;
    for (it = results.begin(); it != results.end(); ++it) {
        ofs << it->first << ".jpg";
        for (unsigned long i = 0; i < it->second.size(); i++)
            ofs << " " << i << " " << it->second.at(i) << ".jpg";
        ofs << std::endl;
    }

    ofs.close();
}
#endif

unsigned long filename_to_id(const char* filename) {
    const char* id = strrchr(filename, '/') + sizeof(char); // +sizeof(char) excludes the slash
    return strtoul(id, NULL, 0);
}

void init(uint8_t** in, size_t* in_len, unsigned nr_clusters, size_t row_len, const char* train_technique) {
    *in_len = sizeof(unsigned char) + sizeof(unsigned) + sizeof(size_t) + strlen(train_technique) + 1;
    *in = (uint8_t*)malloc(*in_len);

    (*in)[0] = OP_IEE_INIT;
    memcpy(*in + sizeof(unsigned char), &nr_clusters, sizeof(unsigned));
    memcpy(*in + sizeof(unsigned char) + sizeof(unsigned), &row_len, sizeof(size_t));
    memcpy(*in + sizeof(unsigned char) + sizeof(unsigned) + sizeof(size_t), train_technique, strlen(train_technique));
    (*in)[sizeof(unsigned char) + sizeof(unsigned) + sizeof(size_t) + strlen(train_technique)] = '\0';
}

void add_train_images(uint8_t** in, size_t* in_len, const descriptor_t descriptor, std::string file_name) {
    unsigned long id = filename_to_id(file_name.c_str());

    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    descriptor->detect(image, keypoints);

    Mat descriptors;
    descriptor->compute(image, keypoints, descriptors);

    const size_t desc_len = (size_t)descriptors.size().width;
    const size_t nr_desc = (size_t)descriptors.size().height;

    float* descriptors_buffer = (float*)malloc(desc_len * nr_desc * sizeof(float));
    for (unsigned i = 0; i < nr_desc; i++) {
        for (size_t j = 0; j < desc_len; j++)
            descriptors_buffer[i * desc_len + j] = *descriptors.ptr<float>(i, j);
    }

    // send
    *in_len = sizeof(unsigned char) + sizeof(unsigned long) + sizeof(size_t) + nr_desc * desc_len * sizeof(float);
    *in = (uint8_t*)malloc(*in_len);
    uint8_t* tmp = *in;

    tmp[0] = OP_IEE_TRAIN_ADD;
    tmp += sizeof(unsigned char);

    memcpy(tmp, &id, sizeof(unsigned long));
    tmp += sizeof(unsigned long);

    memcpy(tmp, &nr_desc, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, descriptors_buffer, nr_desc * desc_len * sizeof(float));

    free(descriptors_buffer);
}

void add_images(uint8_t** in, size_t* in_len, const descriptor_t descriptor, std::string file_name) {
    unsigned long id = filename_to_id(file_name.c_str());

    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    descriptor->detect(image, keypoints);

    Mat descriptors;
    descriptor->compute(image, keypoints, descriptors);

    const size_t desc_len = (size_t)descriptors.size().width;
    const size_t nr_desc = (size_t)descriptors.size().height;

    float* descriptors_buffer = (float*)malloc(desc_len * nr_desc * sizeof(float));
    for (unsigned i = 0; i < nr_desc; i++) {
        for (size_t j = 0; j < desc_len; j++)
            descriptors_buffer[i * desc_len + j] = *descriptors.ptr<float>(i, j);
    }

    // send
    *in_len = sizeof(unsigned char) + sizeof(unsigned long) + sizeof(size_t) + nr_desc * desc_len * sizeof(float);
    *in = (uint8_t*)malloc(*in_len);
    uint8_t* tmp = *in;

    tmp[0] = OP_IEE_ADD;
    tmp += sizeof(unsigned char);

    memcpy(tmp, &id, sizeof(unsigned long));
    tmp += sizeof(unsigned long);

    memcpy(tmp, &nr_desc, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, descriptors_buffer, nr_desc * desc_len * sizeof(float));

    free(descriptors_buffer);
}

void train(uint8_t** in, size_t* in_len) {
    *in_len = sizeof(unsigned char);

    *in = (uint8_t*)malloc(*in_len);
    *in[0] = OP_IEE_TRAIN;
}

void train_lsh(uint8_t** in, size_t* in_len) {
    *in_len = sizeof(unsigned char);

    *in = (uint8_t*)malloc(*in_len);
    *in[0] = OP_IEE_TRAIN_LSH;
}

void train_load_clusters(uint8_t** in, size_t* in_len) {
    *in_len = sizeof(unsigned char);

    *in = (uint8_t*)malloc(*in_len);
    *in[0] = OP_IEE_LOAD;
}

void clear(uint8_t** in, size_t* in_len) {
    *in_len = sizeof(unsigned char);

    *in = (uint8_t*)malloc(*in_len);
    *in[0] = OP_IEE_CLEAR;
}

void dump_bench(uint8_t** in, size_t* in_len) {
    *in_len = sizeof(unsigned char);

    *in = (uint8_t*)malloc(*in_len);
    *in[0] = OP_IEE_DUMP_BENCH;
}

void search(uint8_t** in, size_t* in_len, const descriptor_t descriptor, const std::string file_name) {
    const char* id = strrchr(file_name.c_str(), '/') + sizeof(char); // +sizeof(char) excludes the slash
#if PRINT_DEBUG
    printf(" - Search for %s -\n", id);
#endif
    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    descriptor->detect(image, keypoints);

    Mat descriptors;
    descriptor->compute(image, keypoints, descriptors);

    const size_t desc_len = (size_t)descriptors.size().width;
    const size_t nr_desc = (size_t)descriptors.size().height;

    float* descriptors_buffer = (float*)malloc(desc_len * nr_desc * sizeof(float));

    for (unsigned i = 0; i < nr_desc; ++i) {
        for (size_t j = 0; j < desc_len; ++j)
            descriptors_buffer[i * desc_len + j] = *descriptors.ptr<float>(i, j);
    }
#if PRINT_DEBUG
    printf("nr desc %lu\n", nr_desc);
#endif
    // send
    *in_len = sizeof(unsigned char) + sizeof(size_t) + nr_desc * desc_len * sizeof(float);

    *in = (uint8_t*)malloc(*in_len);
    uint8_t* tmp = *in;

    tmp[0] = OP_IEE_SEARCH;
    tmp += sizeof(unsigned char);

    memcpy(tmp, &nr_desc, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, descriptors_buffer, nr_desc * desc_len * sizeof(float));

    free(descriptors_buffer);
}

void search_test_wang(secure_connection* conn, const descriptor_t descriptor) {
    size_t in_len;
    uint8_t* in;

    map<unsigned long, vector<unsigned long>> precision_res;

    // generate file list to search
    vector<string> files;
    for (int i = 1; i <= 1000; i += 50) {
        char img[128];
        sprintf(img, "/home/guilherme/Datasets/wang/%d.jpg", i);
        files.push_back(img);
    }

    for (size_t i = 0; i < files.size(); ++i) {
        search(&in, &in_len, descriptor, files[i]);
        iee_send(conn, in, in_len);
        free(in);

        // receive response
        size_t res_len;
        uint8_t* res;
        iee_recv(conn, &res, &res_len);

        // get number of response docs
        unsigned nr;
        memcpy(&nr, res, sizeof(unsigned));
        printf("nr of imgs: %u\n", nr);

#if STORE_RESULTS
        // store the results in order
        vector<unsigned long> precision_results;
#endif

        // decode id and score for each doc
        for (unsigned j = 0; j < nr; ++j) {
            unsigned long id;
            memcpy(&id, res + sizeof(size_t) + j * SRC_RES_LEN, sizeof(unsigned long));

            double score;
            memcpy(&score, res + sizeof(size_t) + j * SRC_RES_LEN + sizeof(unsigned long), sizeof(double));

            printf("%lu %f\n", id, score);
#if STORE_RESULTS
            precision_results.push_back(id);
#endif
        }

        free(res);
#if STORE_RESULTS
        precision_res[filename_to_id(files[i].c_str())] = precision_results;
#endif
    }

#if STORE_RESULTS
    // write results to file
    printHolidayResults("results.dat", precision_res);
#endif
}

void search_test(secure_connection* conn, const descriptor_t descriptor, int dbg_limit) {
    struct timeval start, end;
    double total_client = 0, total_iee = 0;
    size_t in_len;
    uint8_t* in;

    map<unsigned long, vector<unsigned long>> precision_res;

    // generate file list to search
    vector<string> files;
    for (int i = 100000; i <= 149900; i += 100) {
        char img[128];
        sprintf(img, "/home/guilherme/Datasets/inria/%d.jpg", i);
        files.push_back(img);
    }

    const size_t query_count = std::min(files.size(), dbg_limit > 0 ? dbg_limit : files.size());
    for (size_t i = 0; i < query_count; ++i) {
        if(i % 20 == 0)
            printf("Search img (%lu/%lu)\n", i, query_count);

        gettimeofday(&start, NULL);
        search(&in, &in_len, descriptor, files[i]);
        gettimeofday(&end, NULL);
        total_client += untrusted_util::time_elapsed_ms(start, end);

        gettimeofday(&start, NULL);
        iee_send(conn, in, in_len);
        free(in);

        // receive response
        size_t res_len;
        uint8_t* res;
        iee_recv(conn, &res, &res_len);
        gettimeofday(&end, NULL);
        total_iee += untrusted_util::time_elapsed_ms(start, end);

        // get number of response docs
        gettimeofday(&start, NULL);
        unsigned nr;
        memcpy(&nr, res, sizeof(unsigned));
#if PRINT_DEBUG
        printf("nr of imgs: %u\n", nr);
#endif

#if STORE_RESULTS
        // store the results in order
        vector<unsigned long> precision_results;
#endif

        // decode id and score for each doc
        for (unsigned j = 0; j < nr; ++j) {
            unsigned long id;
            memcpy(&id, res + sizeof(size_t) + j * SRC_RES_LEN, sizeof(unsigned long));

            double score;
            memcpy(&score, res + sizeof(size_t) + j * SRC_RES_LEN + sizeof(unsigned long), sizeof(double));

#if PRINT_DEBUG
            printf("%lu %f\n", id, score);
#endif
#if STORE_RESULTS
            precision_results.push_back(id);
#endif
        }

        free(res);
        gettimeofday(&end, NULL);
        total_client += untrusted_util::time_elapsed_ms(start, end);
#if STORE_RESULTS
        precision_res[filename_to_id(files[i].c_str())] = precision_results;
#endif
    }

#if STORE_RESULTS
    // write results to file
    printHolidayResults("results.dat", precision_res);
#endif

    printf("-- VISEN TOTAL search: %lf ms %lu queries--\n", total_client + total_iee, query_count);
    printf("-- VISEN search client: %lf ms --\n", total_client);
    printf("-- VISEN search iee w/ net: %lf ms --\n", total_iee);
}
