#ifndef _IMAGESEARCH_H
#define _IMAGESEARCH_H

#include <opencv2/xfeatures2d.hpp>
#include <opencv2/opencv.hpp>
#include <stdlib.h>

#include "untrusted_util.h"
#include "definitions.h"

class feature_extractor {
public:
    feature_extractor(int is_sift, unsigned parameter);
    ~feature_extractor();
    const cv::Mat get_features(cv::Mat image) const;
    const int get_desc_len() const;
private:
    int is_sift = 1;
    //unsigned param = 500;

    cv::Ptr<cv::xfeatures2d::SIFT> sift;
    cv::Ptr<cv::xfeatures2d::SURF> surf;
};

void init(uint8_t** in, size_t* in_len, unsigned nr_clusters, size_t row_len, const char* train_technique);
size_t add_train_images(uint8_t** in, size_t* in_len, const feature_extractor desc, std::string file_name);
void add_images(uint8_t** in, size_t* in_len, const feature_extractor desc, std::string file_name);
void train(uint8_t** in, size_t* in_len);
void train_lsh(uint8_t** in, size_t* in_len);
void train_load_clusters(uint8_t** in, size_t* in_len);
void clear(uint8_t** in, size_t* in_len);
void dump_bench(uint8_t** in, size_t* in_len);
void search(uint8_t** in, size_t* in_len, const feature_extractor desc, const std::string file_name);
void search_test_wang(secure_connection* conn, const feature_extractor desc);
void search_test(secure_connection* conn, const feature_extractor desc, const char* results_file, int dbg_limit);
void search_flickr(secure_connection* conn, const feature_extractor desc, std::vector<std::string> files, int limit);

// benchmark
double get_feature_time_add();
double get_read_time_add();
#endif
