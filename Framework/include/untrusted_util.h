#ifndef __UNTRUSTED_UTIL_H
#define __UNTRUSTED_UTIL_H

#include <stdlib.h>

namespace untrusted_util {
    long time_elapsed_ms(struct timeval start, struct timeval end);

    int socket_connect(const char* server_name, const int server_port);
    int init_server(const int server_port);

    void socket_send(int socket, const void* buff, size_t len);
    void socket_receive(int socket, void* buff, size_t len);
}

#endif
