#include "untrusted_util.h"

#include <sys/time.h>
#include <time.h>

long untrusted_util::time_elapsed(struct timeval start, struct timeval end) {
    long secs_used, micros_used;

    secs_used = (end.tv_sec - start.tv_sec); //avoid overflow by subtracting first
    micros_used = ((secs_used*1000000) + end.tv_usec) - (start.tv_usec);
    return micros_used;
}
