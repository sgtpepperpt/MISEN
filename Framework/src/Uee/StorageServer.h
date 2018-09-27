#ifndef __UEE_SERVER_H__
#define __UEE_SERVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
//#include <unordered_map>
#include <errno.h>
#include <err.h>
#include <signal.h>

#include "tbb/concurrent_unordered_map.h"

#include "MapUtil.h"

using namespace std;

// map label and value sizes
const size_t l_size = 32;
const size_t d_size = 44;

const size_t pair_len = l_size + d_size;

#endif
