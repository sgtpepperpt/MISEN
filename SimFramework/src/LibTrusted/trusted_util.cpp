#include "trusted_util.h"

#include <stdio.h>

long trusted_util::time_elapsed_ms(untrusted_time start, untrusted_time end) {
    long secs_used, micros_used;

    secs_used = (end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
    micros_used = ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
    return micros_used / 1000;
}

// FILE*
void* trusted_util::open_secure(const char* name, const char* mode) {
    return fopen(name, mode);
}

size_t trusted_util::write_secure(const void* ptr, size_t size, size_t count, void* stream) {
    return fwrite(ptr, size, count, (FILE*)stream);
}

size_t trusted_util::read_secure(void* ptr, size_t size, size_t count, void* stream) {
    return fread(ptr, size, count, (FILE*)stream);
}

void trusted_util::close_secure(void* stream) {
    fclose((FILE*)stream);
}
