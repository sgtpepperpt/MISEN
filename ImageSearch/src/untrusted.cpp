#include "internal_untrusted.h"

void untrusted_process_message(void** out, size_t* out_len, const void* in, const size_t in_len) {
    *out_len = 0;
    *out = NULL;

    switch(((unsigned char*)in)[0]) {
        case 'a': {
            *out_len = 10;
            *out = malloc(*out_len);
            memset(*out, 0x00, *out_len);

            ((unsigned char*)*out)[0] = 'z';
            break;
        }
    }
}
