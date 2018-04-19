#include "untrusted_util.h"

#include <stdarg.h>
#include <stdio.h>     /* vsnprintf */

#include "Enclave_t.h" /* ocalls */
#include "types.h"

/****************************************************** FILE I/O ******************************************************/
int untrusted_util::open(const char *filename, int mode) {
    int retval;
    sgx_status_t sgx_ret = ocall_open(&retval, filename, mode);
    if(sgx_ret != SGX_SUCCESS) {
        untrusted_util::printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return retval;
}

ssize_t untrusted_util::read(int file, void *buf, size_t len) {
    ssize_t retval;
    sgx_status_t sgx_ret = ocall_read(&retval, file, buf, len);
    if(sgx_ret != SGX_SUCCESS) {
        untrusted_util::printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return retval;
}

ssize_t untrusted_util::write(int file, void *buf, size_t len) {
    ssize_t retval;
    sgx_status_t sgx_ret = ocall_write(&retval, file, buf, len);
    if(sgx_ret != SGX_SUCCESS) {
        untrusted_util::printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return retval;
}

void untrusted_util::close(int file) {
    ocall_close(file);
}
/**************************************************** END FILE I/O ****************************************************/

/******************************************************** MISC ********************************************************/
void untrusted_util::printf(const char *fmt, ...) {
    //ocall_print_string("Enclave printf\n");
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

void untrusted_util::exit(int status) {
    ocall_exit(status);
}
/****************************************************** END MISC ******************************************************/

int untrusted_util::open_uee_connection() {
    int retval;
    sgx_status_t sgx_ret = ocall_open_uee_connection(&retval);
    if(sgx_ret != SGX_SUCCESS) {
        untrusted_util::printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return retval;
}

void untrusted_util::uee_process(const int socket, void **out, size_t *out_len, const void *in, const size_t in_len) {
    sgx_status_t sgx_ret = ocall_uee_process(socket, out, out_len, in, in_len);
    if(sgx_ret != SGX_SUCCESS) {
        untrusted_util::printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }
}

void untrusted_util::close_uee_connection(const int socket) {
    sgx_status_t sgx_ret = ocall_close_uee_connection(socket);
    if(sgx_ret != SGX_SUCCESS) {
        untrusted_util::printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }
}

/***************************************************** ALLOCATORS *****************************************************/
void* untrusted_util::outside_malloc(size_t length){
    void* return_pointer;
    sgx_status_t sgx_ret = ocall_untrusted_malloc(&return_pointer, length);
    if(sgx_ret != SGX_SUCCESS) {
        untrusted_util::printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return return_pointer;
}

void untrusted_util::outside_free(void *pointer){
    sgx_status_t sgx_ret = ocall_untrusted_free(&pointer);
    if(sgx_ret != SGX_SUCCESS) {
        untrusted_util::printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }
}

untrusted_time untrusted_util::curr_time() {
    untrusted_time t;
    sgx_status_t sgx_ret = ocall_curr_time(&t);
    if(sgx_ret != SGX_SUCCESS) {
        untrusted_util::printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return t;
}
/*************************************************** END ALLOCATORS ***************************************************/

/****************************************************** GENERIC ******************************************************/
int untrusted_util::process(void **out, size_t *out_len, const void *in, const size_t in_len) {
    int retval;
    sgx_status_t sgx_ret = ocall_process(&retval, out, out_len, in, in_len);
    if(sgx_ret != SGX_SUCCESS) {
        untrusted_util::printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return retval;
}
/**************************************************** END GENERIC ****************************************************/