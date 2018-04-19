#ifndef __EXTERN_LIB_H_
#define __EXTERN_LIB_H_

#include <stdlib.h>
#include <stdint.h>

// external lib should implement these
namespace extern_lib {
    void init(); // called before threading is initialised
    void thread_do_task(void* args); // task to be done by a thread
    void process_message(uint8_t** out, size_t* out_len, const uint8_t* in, const size_t in_len); // process a message from the client
}

namespace extern_lib_ut {
    void process_message(void** out, size_t* out_len, const void* in, const size_t in_len); // process a generic ocall performed from extern_lib
}

#endif
