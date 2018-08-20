#ifndef VISEN_MISEN_TESTS_H
#define VISEN_MISEN_TESTS_H

#include <stdlib.h>
#include <vector>

#include "mbedtls/ssl.h"
#include "ImageSearch.h"
#include "bisen_tests.h"

void misen_search(mbedtls_ssl_context* ssl, SseClient* client, cv::Ptr<cv::xfeatures2d::SIFT> extractor, std::vector<std::pair<std::string, std::string>> queries);
std::vector<std::pair<std::string, std::string>> generate_multimodal_queries(unsigned nr_docs);

#endif
