#include "internal_ocall_wrapper.h"

/****************************************************** FILE I/O ******************************************************/
int sgx_open(const char* filename, int mode) {
    int retval;
    sgx_status_t sgx_ret = ocall_open(&retval, filename, mode);
    if(sgx_ret != SGX_SUCCESS) {
        sgx_printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return retval;
}

ssize_t sgx_read(int file, void *buf, size_t len) {
    ssize_t retval;
    sgx_status_t sgx_ret = ocall_read(&retval, file, buf, len);
    if(sgx_ret != SGX_SUCCESS) {
        sgx_printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return retval;
}

ssize_t sgx_write(int file, void *buf, size_t len) {
    ssize_t retval;
    sgx_status_t sgx_ret = ocall_write(&retval, file, buf, len);
    if(sgx_ret != SGX_SUCCESS) {
        sgx_printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return retval;
}

void sgx_close(int file) {
    ocall_close(file);
}
/**************************************************** END FILE I/O ****************************************************/

/******************************************************** MISC ********************************************************/
void sgx_printf(const char *fmt, ...) {
    //ocall_print_string("Enclave printf\n");
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}

void sgx_exit(int status) {
    ocall_exit(status);
}
/****************************************************** END MISC ******************************************************/

int sgx_open_uee_connection() {
    int retval;
    sgx_status_t sgx_ret = ocall_open_uee_connection(&retval);
    if(sgx_ret != SGX_SUCCESS) {
        sgx_printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return retval;
}

void sgx_uee_process(const int socket, void** out, size_t* out_len, const void* in, const size_t in_len) {
    sgx_status_t sgx_ret = ocall_uee_process(socket, out, out_len, in, in_len);
    if(sgx_ret != SGX_SUCCESS) {
        sgx_printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }
}

void sgx_close_uee_connection(const int socket) {
    sgx_status_t sgx_ret = ocall_close_uee_connection(socket);
    if(sgx_ret != SGX_SUCCESS) {
        sgx_printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }
}

/***************************************************** ALLOCATORS *****************************************************/
void* sgx_untrusted_malloc(size_t length){
    void* return_pointer;
    sgx_status_t sgx_ret = ocall_untrusted_malloc(&return_pointer, length);
    if(sgx_ret != SGX_SUCCESS) {
        sgx_printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return return_pointer;
}

void sgx_untrusted_free(void* pointer){
    sgx_status_t sgx_ret = ocall_untrusted_free(&pointer);
    if(sgx_ret != SGX_SUCCESS) {
        sgx_printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }
}

untrusted_time sgx_curr_time() {
    untrusted_time t;
    sgx_status_t sgx_ret = ocall_curr_time(&t);
    if(sgx_ret != SGX_SUCCESS) {
        sgx_printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return t;
}
/*************************************************** END ALLOCATORS ***************************************************/

/****************************************************** GENERIC ******************************************************/
int sgx_ocall_process(void** out, size_t* out_len, const void* in, const size_t in_len) {
    int retval;
    sgx_status_t sgx_ret = ocall_process(&retval, out, out_len, in, in_len);
    if(sgx_ret != SGX_SUCCESS) {
        sgx_printf("OCALL ERROR ON RETURN: %ld\n", sgx_ret);
        ocall_exit(-1);
    }

    return retval;
}
/**************************************************** END GENERIC ****************************************************/
