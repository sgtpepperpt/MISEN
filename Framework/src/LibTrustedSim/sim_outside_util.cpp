#include "outside_util.h"

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#include "definitions.h"
#include "extern_lib.h"
#include "untrusted_util.h"
#include "types.h"


int outside_util::open(const char *filename, int mode) {
    return open(filename, mode);
}

ssize_t outside_util::read(int file, void *buf, size_t len) {
    return read(file, buf, len);
}

ssize_t outside_util::write(const int file, const void *buf, const size_t len) {
    return write(file, buf, len);
}

void outside_util::close(int file) {
    close(file);
}

void outside_util::printf(const char *fmt, ...) {
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    printf("%s", buf);
}

void outside_util::exit(int status) {
    exit(status);
}

int outside_util::open_uee_connection() {
    const char* host = UEE_HOSTNAME;
    const int port = UEE_PORT;

    return untrusted_util::socket_connect(host, port);
}

void outside_util::uee_process(const int socket, void **out, size_t *out_len, const void *in, const size_t in_len) {
    untrusted_util::socket_send(socket, &in_len, sizeof(size_t));
    untrusted_util::socket_send(socket, in, in_len);

    untrusted_util::socket_receive(socket, out_len, sizeof(size_t));
    *out = malloc(*out_len);
    untrusted_util::socket_receive(socket, *out, *out_len);
}

void outside_util::close_uee_connection(const int socket) {
    close(socket);
}

void* outside_util::outside_malloc(size_t length){
    return malloc(length);
}

void outside_util::outside_free(void *pointer){
    free(pointer);
}

untrusted_time outside_util::curr_time() {
    timeval curr;
    gettimeofday(&curr, NULL);

    untrusted_time t;
    t.tv_sec = curr.tv_sec;
    t.tv_usec = curr.tv_usec;

    return t;
}

int outside_util::process(void **out, size_t *out_len, const void *in, const size_t in_len) {
    extern_lib_ut::process_message(out, out_len, in, in_len); // must be implemented by extern lib
}
