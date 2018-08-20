#ifndef __CLIENT_TRAINING_H
#define __CLIENT_TRAINING_H

#include "untrusted_util.h"
#include "rbisen/SseClient.hpp"

#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>

void visen_setup(secure_connection* conn, size_t desc_len, unsigned visen_nr_clusters);
void visen_train_client_kmeans(secure_connection* conn, size_t desc_len, unsigned visen_nr_clusters, char* visen_train_mode, char* visen_centroids_file, const char* visen_dataset_dir, cv::Ptr<cv::xfeatures2d::SIFT> extractor);
void visen_train_iee_kmeans(secure_connection* conn, char* visen_train_mode, cv::Ptr<cv::xfeatures2d::SIFT> extractor, const vector<string> files);
void visen_add_files(secure_connection* conn, cv::Ptr<cv::xfeatures2d::SIFT> extractor, const vector<string> files);

#endif
