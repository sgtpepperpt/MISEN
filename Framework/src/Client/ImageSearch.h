#ifndef SGX_IMAGE_SEARCH_IMAGESEARCH_H
#define SGX_IMAGE_SEARCH_IMAGESEARCH_H

#include <opencv2/xfeatures2d.hpp>
#include <opencv2/opencv.hpp>
#include <stdlib.h>

#include "untrusted_util.h"
#include "definitions.h"

#define SRC_RES_LEN (sizeof(unsigned long) + sizeof(double))

#if DESCRIPTOR == DESC_SIFT
typedef cv::Ptr<cv::xfeatures2d::SIFT> descriptor_t;
#else
typedef cv::Ptr<cv::xfeatures2d::SURF> descriptor_t;
#endif

descriptor_t create_descriptor(int parameter);

void init(uint8_t** in, size_t* in_len, unsigned nr_clusters, size_t row_len, const char* train_technique);
void add_train_images(uint8_t** in, size_t* in_len, const descriptor_t descriptor, std::string file_name);
void add_images(uint8_t** in, size_t* in_len, const descriptor_t descriptor, std::string file_name);
void train(uint8_t** in, size_t* in_len);
void train_lsh(uint8_t** in, size_t* in_len);
void train_load_clusters(uint8_t** in, size_t* in_len);
void clear(uint8_t** in, size_t* in_len);
void dump_bench(uint8_t** in, size_t* in_len);
void search(uint8_t** in, size_t* in_len, const descriptor_t descriptor, const std::string file_name);
void search_test_wang(secure_connection* conn, const descriptor_t descriptor);
void search_test(secure_connection* conn, const descriptor_t descriptor, const char* results_file, int dbg_limit);

#endif
