#ifndef SGX_IMAGE_SEARCH_IMAGESEARCH_H
#define SGX_IMAGE_SEARCH_IMAGESEARCH_H

#include <opencv2/xfeatures2d.hpp>
#include <opencv2/opencv.hpp>
#include <stdlib.h>

#include "untrusted_util.h"

#define SRC_RES_LEN (sizeof(unsigned long) + sizeof(double))

void init(uint8_t** in, size_t* in_len, unsigned nr_clusters, size_t row_len);
void add_train_images(uint8_t** in, size_t* in_len, const cv::Ptr<cv::xfeatures2d::SIFT> surf, std::string file_name);
void add_images(uint8_t** in, size_t* in_len, const cv::Ptr<cv::xfeatures2d::SIFT> surf, std::string file_name);
void add_images_lsh(uint8_t** in, size_t* in_len, const cv::Ptr<cv::xfeatures2d::SIFT> surf, std::string file_name);
void train(uint8_t** in, size_t* in_len);
void train_lsh(uint8_t** in, size_t* in_len);
void train_load_clusters(uint8_t** in, size_t* in_len);
void clear(uint8_t** in, size_t* in_len);
void search(uint8_t** in, size_t* in_len, const cv::Ptr<cv::xfeatures2d::SIFT> surf, const std::string file_name);
void search_test_wang(secure_connection* conn, const cv::Ptr<cv::xfeatures2d::SIFT> surf);
void search_test(secure_connection* conn, const cv::Ptr<cv::xfeatures2d::SIFT> extractor);

#endif
