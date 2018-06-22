//
// Created by Guilherme Borges on 22/06/2018.
//

#ifndef TRUSTED_MBEDTLS_H
#define TRUSTED_MBEDTLS_H

#include "mbedtls/ssl.h"
#include "mbedtls/net.h"

typedef struct {
    mbedtls_net_context client_fd;
    int thread_complete;
    mbedtls_ssl_config *config;
} thread_info_t;

#endif //SGX_IMAGE_SEARCH_TRUSTED_MBEDTLS_H
