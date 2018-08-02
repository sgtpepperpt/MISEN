#include "ImageSearch.h"

#include "definitions.h"
#include "untrusted_util_tls.h"

#include <dirent.h>
#include <map>
#include <fstream>

using namespace cv;
using namespace cv::xfeatures2d;
using namespace std;

#define STORE_RESULTS 1
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

std::vector<std::string> get_filenames(int n, string dataset_path) {
    vector<string> filenames;

    DIR* dir;
    struct dirent* ent;
    int i = 0;
    if ((dir = opendir(dataset_path.c_str()))) {
        while ((ent = readdir(dir)) != NULL && (n < 0 || i < n)) {
            std::string fname = dataset_path + "/" + ent->d_name;
            if (fname.find(".jpg") == string::npos || !fname.length())
                continue;

            //printf("%s\n", fname.c_str());
            filenames.push_back(fname);
            i++;
        }
        closedir(dir);
    }

    return filenames;
}

void iee_send(mbedtls_ssl_context* ssl, const uint8_t* in, const size_t in_len) {
    //printf("will send %lu\n", in_len);
    untrusted_util_tls::socket_send(ssl, &in_len, sizeof(size_t));
    untrusted_util_tls::socket_send(ssl, in, in_len);
}

void iee_recv(mbedtls_ssl_context* ssl, uint8_t** out, size_t* out_len) {
    untrusted_util_tls::socket_receive(ssl, out_len, sizeof(size_t));

    //printf("will receive %lu\n", *out_len);

    *out = (uint8_t*)malloc(*out_len);
    untrusted_util_tls::socket_receive(ssl, *out, *out_len);
}

void iee_comm(mbedtls_ssl_context* ssl, const void* in, const size_t in_len) {
    iee_send(ssl, (uint8_t*)in, in_len);
    size_t res_len;
    unsigned char* res;
    iee_recv(ssl, &res, &res_len);

    //printf("res: %lu bytes\n", res_len);
    free(res);
}

void init(uint8_t** in, size_t* in_len, unsigned nr_clusters, size_t row_len) {
    *in_len = sizeof(unsigned char) + sizeof(unsigned) + sizeof(size_t);
    *in = (uint8_t*)malloc(*in_len);

    *in[0] = OP_IEE_INIT;
    memcpy(*in + sizeof(unsigned char), &nr_clusters, sizeof(unsigned));
    memcpy(*in + sizeof(unsigned char) + sizeof(unsigned), &row_len, sizeof(size_t));
}

void add_train_images(uint8_t** in, size_t* in_len, const Ptr<SIFT> surf, std::string file_name) {
    unsigned long id = filename_to_id(file_name.c_str());

    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    surf->detect(image, keypoints);

    Mat descriptors;
    surf->compute(image, keypoints, descriptors);

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

void add_images(uint8_t** in, size_t* in_len, const Ptr<SIFT> surf, std::string file_name) {
    unsigned long id = filename_to_id(file_name.c_str());

    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    surf->detect(image, keypoints);

    Mat descriptors;
    surf->compute(image, keypoints, descriptors);

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

void add_images_lsh(uint8_t** in, size_t* in_len, const Ptr<SIFT> surf, std::string file_name) {
    unsigned long id = filename_to_id(file_name.c_str());

    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    surf->detect(image, keypoints);

    Mat descriptors;
    surf->compute(image, keypoints, descriptors);

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

    tmp[0] = OP_IEE_ADD_LSH;
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

void search(uint8_t** in, size_t* in_len, const Ptr<SIFT> surf, const std::string file_name) {
    const char* id = strrchr(file_name.c_str(), '/') + sizeof(char); // +sizeof(char) excludes the slash
    printf(" - Search for %s -\n", id);

    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    surf->detect(image, keypoints);

    Mat descriptors;
    surf->compute(image, keypoints, descriptors);

    const size_t desc_len = (size_t)descriptors.size().width;
    const size_t nr_desc = (size_t)descriptors.size().height;

    float* descriptors_buffer = (float*)malloc(desc_len * nr_desc * sizeof(float));

    for (unsigned i = 0; i < nr_desc; ++i) {
        for (size_t j = 0; j < desc_len; ++j)
            descriptors_buffer[i * desc_len + j] = *descriptors.ptr<float>(i, j);
    }

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

void search_test_wang(mbedtls_ssl_context* ssl, const Ptr<SIFT> surf) {
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
        search(&in, &in_len, surf, files[i]);
        iee_send(ssl, in, in_len);
        free(in);

        // receive response
        size_t res_len;
        uint8_t* res;
        iee_recv(ssl, &res, &res_len);

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
        precision_res[filename_to_id(files[i].c_str())] = precision_results;
    }

#if STORE_RESULTS
    // write results to file
    printHolidayResults("results.dat", precision_res);
#endif
}

void search_test(mbedtls_ssl_context* ssl, const Ptr<SIFT> surf) {
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

    for (size_t i = 0; i < files.size(); ++i) {
        search(&in, &in_len, surf, files[i]);
        iee_send(ssl, in, in_len);
        free(in);

        // receive response
        size_t res_len;
        uint8_t* res;
        iee_recv(ssl, &res, &res_len);

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
        precision_res[filename_to_id(files[i].c_str())] = precision_results;
    }

#if STORE_RESULTS
    // write results to file
    printHolidayResults("results.dat", precision_res);
#endif
}
