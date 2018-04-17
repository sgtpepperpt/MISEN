#ifndef _TIME_UTIL_H
#define _TIME_UTIL_H

#include <sys/time.h>
#include <time.h>

long util_time_elapsed(struct timeval start, struct timeval end);

#endif
