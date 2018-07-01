#ifndef __UNTRUSTED_UTIL_TLS_H
#define __UNTRUSTED_UTIL_TLS_H

#include <stdlib.h>
#include <stdint.h>

#include "mbedtls/ssl.h"

namespace untrusted_util_tls {
    void socket_send(mbedtls_ssl_context* ssl, const void* buff, size_t len);
    void socket_receive(mbedtls_ssl_context* ssl, void* buff, size_t len);
}

#endif
