#ifndef VISEN_BISEN_TESTS_H
#define VISEN_BISEN_TESTS_H

#include "mbedtls/ssl.h"
#include "rbisen/SseClient.hpp"

void bisen_setup(mbedtls_ssl_context* ssl, SseClient* client);
void bisen_update(mbedtls_ssl_context* ssl, SseClient* client, char* bisen_doc_type, unsigned nr_docs, std::vector<std::string> doc_paths);
void bisen_search(mbedtls_ssl_context* ssl, SseClient* client, vector<string> queries);

#endif
