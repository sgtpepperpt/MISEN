#ifndef __CLIENT_TRAINING_H
#define __CLIENT_TRAINING_H

#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>

float* client_train(const char* path, const unsigned nr_clusters, const unsigned desc_len, cv::Ptr<cv::xfeatures2d::SIFT> descriptor);

#endif
