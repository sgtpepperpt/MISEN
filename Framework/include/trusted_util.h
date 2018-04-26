#ifndef __TRUSTED_UTIL_H
#define __TRUSTED_UTIL_H

#include "types.h"

namespace trusted_util {
    // threading
    const unsigned thread_get_count();
    int thread_add_work(void* (*task)(void*), void* args);
    void thread_do_work();

    // time
    long time_elapsed_ms(untrusted_time start, untrusted_time end);
}

#endif
