#ifndef _CLIENT_H
#define _CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <dirent.h>

#include "net_util.h"

typedef struct img_descriptor {
    unsigned count;
    float* descriptors;
} img_descriptor;

#endif
