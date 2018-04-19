#ifndef __TRUSTED_UTIL_H
#define __TRUSTED_UTIL_H

namespace trusted_util {
    // threading
    unsigned thread_get_count();
    int thread_add_work(void* args);
    void thread_do_work();
}

#endif
