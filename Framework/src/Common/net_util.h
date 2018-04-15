#ifndef _NET_UTIL_H
#define _NET_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int socket_connect(const char* server_name, const int server_port);
int init_server(const int server_port);

void socket_send(int socket, const void* buff, size_t len);
void socket_receive(int socket, void* buff, size_t len);

#endif
