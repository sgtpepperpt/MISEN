#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <iostream>
#include <atomic>
#include <csignal>

#include "mbedtls/ssl.h"
#include "mbedtls/net.h"
#include "mbedtls/error.h"
#include "Enclave_u.h"
#include "Utils.h"

#include <sgx_urts.h>
#include <thread>

using std::cerr;
using std::endl;

typedef struct {
    int active;
    thread_info_t data;
    pthread_t thread;
} pthread_info_t;

#define MAX_NUM_THREADS 5

static pthread_info_t threads[MAX_NUM_THREADS];
sgx_enclave_id_t eid;

#include "mbedtls_net.c"

// thread function
void* ecall_handle_ssl_connection(void* data) {
    long int thread_id = pthread_self();
    thread_info_t* thread_info = (thread_info_t*)data;
    ssl_conn_handle(eid, thread_id, thread_info);

    printf("thread exiting for thread %ld\n", thread_id);
    mbedtls_net_free(&thread_info->client_fd);
    return (NULL);
}

static int thread_create(mbedtls_net_context* client_fd) {
    int ret, i;

    for (i = 0; i < MAX_NUM_THREADS; i++) {
        if (threads[i].active == 0)
            break;

        if (threads[i].data.thread_complete == 1) {
            printf("  [ main ]  Cleaning up thread %d\n", i);
            pthread_join(threads[i].thread, NULL);
            memset(&threads[i], 0, sizeof(pthread_info_t));
            break;
        }
    }

    if (i == MAX_NUM_THREADS)
        return (-1);

    threads[i].active = 1;
    threads[i].data.config = NULL;
    threads[i].data.thread_complete = 0;
    memcpy(&threads[i].data.client_fd, client_fd, sizeof(mbedtls_net_context));

    if ((ret = pthread_create(&threads[i].thread, NULL, ecall_handle_ssl_connection, &threads[i].data)) != 0) {
        return (ret);
    }

    return (0);
}
/*
int main(void) {
    int ret;

    if (0 != initialize_enclave(&eid)) {
        printf("failed to init enclave\n");
        exit(-1);
    }

    mbedtls_net_context listen_fd, client_fd;

    // initialize the object
    ssl_conn_init(eid);

    // initialize threads
    memset(threads, 0, sizeof(threads));

    printf("  . Bind on https://localhost:4433/ ...");
    fflush(stdout);

    if ((ret = mbedtls_net_bind(&listen_fd, NULL, "4433", MBEDTLS_NET_PROTO_TCP)) != 0) {
        printf(" failed\n  ! mbedtls_net_bind returned %d\n\n", ret);
        exit(-1);
    }

    printf(" ok\n");
    printf("  [ main ]  Waiting for a remote connection\n");

    // non-block accept
    while (true) {
#ifdef MBEDTLS_ERROR_C
        if (ret != 0) {
          char error_buf[100];
          mbedtls_strerror(ret, error_buf, 100);
          printf("  [ main ]  Last error was: -0x%04x - %s\n", -ret, error_buf);
        }
#endif


        // 3. Wait until a client connects

        if (0 != mbedtls_net_set_nonblock(&listen_fd)) {
            printf("can't set nonblock for the listen socket\n");
        }
        ret = mbedtls_net_accept(&listen_fd, &client_fd, NULL, 0, NULL);
        if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
            ret = 0;
            continue;
        } else if (ret != 0) {
            printf("  [ main ] failed: mbedtls_net_accept returned -0x%04x\n", ret);
            break;
        }

        printf("  [ main ]  ok\n");
        printf("  [ main ]  Creating a new thread for client %d\n", client_fd.fd);

        if ((ret = thread_create(&client_fd)) != 0) {
            printf("  [ main ]  failed: thread_create returned %d\n", ret);
            mbedtls_net_free(&client_fd);
            continue;
        }
        ret = 0;
    }

    sgx_destroy_enclave(eid);
    return (ret);
}
*/
