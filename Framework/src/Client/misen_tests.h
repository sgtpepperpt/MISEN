#ifndef VISEN_MISEN_TESTS_H
#define VISEN_MISEN_TESTS_H

#include <stdlib.h>
#include <vector>

#include "ImageSearch.h"
#include "bisen_tests.h"
#include "untrusted_util.h"

void misen_search(secure_connection* conn, SseClient* client, descriptor_t descriptor, std::vector<std::pair<std::string, std::string>> queries);
std::vector<std::pair<std::string, std::string>> generate_multimodal_queries(vector<string> txt_paths, vector<string> img_paths, int nr_queries);

#endif
