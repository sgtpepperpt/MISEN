#include "untrusted_util_tls.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"

static int ssl_send_all(mbedtls_ssl_context* ssl, const void* buf, size_t len) {
    size_t total = 0;       // how many bytes we've sent
    size_t bytesleft = len; // how many we have left to send
    long n = 0;

    while(total < len) {
        n = mbedtls_ssl_write(ssl, (unsigned char*)buf + total, bytesleft);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    return n == -1 || total != len ? -1 : 0; // return -1 on failure, 0 on success
}

static ssize_t ssl_receive_all(mbedtls_ssl_context* ssl, void* buff, size_t len) {
    ssize_t r = 0;
    while ((unsigned long)r < len) {
        ssize_t n = mbedtls_ssl_read(ssl, (unsigned char*)buff + r, len-r);
        if (n < 0) {
            printf("ERROR reading from socket\n");
            exit(1);
        }
        r+=n;
    }

    return r;
}

void untrusted_util_tls::socket_send(mbedtls_ssl_context* ssl, const void* buff, size_t len) {
    if (ssl_send_all(ssl, buff, len) < 0) {
        printf("ERROR writing to socket\n");
        exit(1);
    }
}

void untrusted_util_tls::socket_receive(mbedtls_ssl_context* ssl, void* buff, size_t len) {
    if (ssl_receive_all(ssl, buff, len) < 0) {
        printf("EERROR reading from socket\n");
        exit(1);
    }
}
