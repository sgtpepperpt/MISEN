#include "outside_util.h"

#include <stdarg.h>
#include <stdio.h>     /* vsnprintf */

#include "ocall.h" /* ocalls */
#include "types.h"
#include "untrusted_util.h"

/****************************************************** FILE I/O ******************************************************/
int outside_util::open(const char *filename, int mode) {
    return ocall_open(filename, mode);
}

ssize_t outside_util::read(int file, void *buf, size_t len) {
    return ocall_read(file, buf, len);
}

ssize_t outside_util::write(const int file, const void *buf, const size_t len) {
    return ocall_write(file, buf, len);
}

void outside_util::close(int file) {
    ocall_close(file);
}
/**************************************************** END FILE I/O ****************************************************/

int outside_util::open_socket(const char* addr, int port) {
    return untrusted_util::socket_connect(addr, port);
}

void outside_util::socket_send(int socket, const void* buff, size_t len) {
    untrusted_util::socket_send(socket, buff, len);
}

void outside_util::socket_receive(int socket, void* buff, size_t len) {
    untrusted_util::socket_receive(socket, buff, len);
}

size_t bytes_sent = 0, bytes_received = 0;

void outside_util::reset_bytes() {
    bytes_sent = 0;
    bytes_received = 0;
}

void outside_util::print_bytes(const char* msg) {
    outside_util::printf("Sent bytes %s iee: %lu\nReceived bytes iee: %lu\n", msg, bytes_sent, bytes_received);
}

/******************************************************** MISC ********************************************************/
void outside_util::printf(const char *fmt, ...) {
    //ocall_string_print("Enclave printf\n");
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_string_print(buf);
}

void outside_util::exit(int status) {
    ocall_exit(status);
}
/****************************************************** END MISC ******************************************************/

int outside_util::open_uee_connection() {
    return ocall_open_uee_connection();
}

void outside_util::uee_process(const int socket, void **out, size_t *out_len, const void *in, const size_t in_len) {
    ocall_uee_process(socket, out, out_len, in, in_len);
}

void outside_util::close_uee_connection(const int socket) {
    ocall_close_uee_connection(socket);
}

/***************************************************** ALLOCATORS *****************************************************/
void* outside_util::outside_malloc(size_t length){
    return ocall_untrusted_malloc(length);
}

void outside_util::outside_free(void* pointer){
    ocall_untrusted_free((size_t)pointer);
}

untrusted_time outside_util::curr_time() {
    return ocall_curr_time();
}
/*************************************************** END ALLOCATORS ***************************************************/

/****************************************************** GENERIC ******************************************************/
int outside_util::process(void **out, size_t *out_len, const void *in, const size_t in_len) {
    return ocall_process(out, out_len, in, in_len);
}
/**************************************************** END GENERIC ****************************************************/
