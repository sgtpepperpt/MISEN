#ifndef __UNTRUSTED_UTIL_H
#define __UNTRUSTED_UTIL_H

#include <stdlib.h>
#include <stdint.h>

namespace untrusted_util {
    long time_elapsed_ms(struct timeval start, struct timeval end);
    void debug_printbuf(uint8_t* buf, size_t len);

    int socket_connect(const char* server_name, const int server_port);
    int init_server(const int server_port);

    void socket_send(int socket, const void* buff, size_t len);
    void socket_receive(int socket, void* buff, size_t len);
}

#endif
