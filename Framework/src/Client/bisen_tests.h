#ifndef VISEN_BISEN_TESTS_H
#define VISEN_BISEN_TESTS_H

#include "mbedtls/ssl.h"
#include "rbisen/SseClient.hpp"

void bisen_setup(mbedtls_ssl_context* ssl, SseClient* client);
void bisen_update(mbedtls_ssl_context* ssl, SseClient* client, unsigned bisen_nr_docs, char* bisen_doc_type, const char* dataset_dir);
void bisen_search(mbedtls_ssl_context* ssl, SseClient* client, vector<string> queries);

#endif
