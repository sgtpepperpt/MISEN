#include "untrusted_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct secure_conn_t {
    int socket;
} secure_conn_t;

void untrusted_util::init_secure_connection(secure_connection** connection, const char* server_name, const int server_port) {
    secure_conn_t* conn = (secure_conn_t*)malloc(sizeof(secure_conn_t));
    *connection = (secure_connection*)conn;

    conn->socket = untrusted_util::socket_connect(server_name, server_port);
}

void untrusted_util::socket_secure_send(secure_connection* connection, const void* buff, size_t len) {
    secure_conn_t* conn = (secure_conn_t*)connection;
    untrusted_util::socket_send(conn->socket, buff, len);
}

void untrusted_util::socket_secure_receive(secure_connection* connection, void* buff, size_t len) {
    secure_conn_t* conn = (secure_conn_t*)connection;
    untrusted_util::socket_receive(conn->socket, buff, len);
}

void untrusted_util::close_secure_connection(secure_connection* connection) {
    secure_conn_t* conn = (secure_conn_t*)connection;
    close(conn->socket);
    free(connection);
}
