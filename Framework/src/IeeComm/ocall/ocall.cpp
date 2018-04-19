#include "ocall.h"
#include "extern_lib.h"

/****************************************************** FILE I/O ******************************************************/
int ocall_open(const char* filename, int mode) {
    printf("ocall open in app\n");
    return open(filename, mode);
}

ssize_t ocall_read(int file, void *buf, size_t len) {
    return read(file, buf, len);
}

ssize_t ocall_write(int file, void *buf, size_t len) {
    return write(file, buf, len);
}

void ocall_close(int file) {
    close(file);
}
/**************************************************** END FILE I/O ****************************************************/

/******************************************************** MISC ********************************************************/
void ocall_print_string(const char *str) {
    printf("%s", str);
}

void ocall_exit(int code) {
    exit(code);
}

untrusted_time ocall_curr_time() {
    timeval curr;
    gettimeofday(&curr, NULL);

    untrusted_time t;
    t.tv_sec = curr.tv_sec;
    t.tv_usec = curr.tv_usec;

    return t;
}
/****************************************************** END MISC ******************************************************/

int ocall_open_uee_connection() {
    const char* host = "localhost"; // TODO both coming from definitions file
    const int port = 7911;

    return socket_connect(host, port);
}

void ocall_uee_process(const int socket, void** out, size_t* out_len, const void* in, const size_t in_len) {
    socket_send(socket, &in_len, sizeof(size_t));
    socket_send(socket, in, in_len);

    socket_receive(socket, out_len, sizeof(size_t));
    *out = malloc(*out_len);
    socket_receive(socket, *out, *out_len);
}

void ocall_close_uee_connection(const int socket) {
    close(socket);
}

/***************************************************** ALLOCATORS *****************************************************/
void* ocall_untrusted_malloc(size_t length) {
    return malloc(length);
}

void ocall_untrusted_free(void** pointer) {
    free(*pointer);
    *pointer = NULL;
}
/*************************************************** END ALLOCATORS ***************************************************/

/****************************************************** GENERIC ******************************************************/
int ocall_process(void** out, size_t* out_len, const void* in, const size_t in_len) {
    extern_lib_ut::process_message(out, out_len, in, in_len);
    return 0;
}
/**************************************************** END GENERIC ****************************************************/
