#ifndef VISEN_UTIL_H
#define VISEN_UTIL_H

#include <stdlib.h>
#include <vector>
#include <string>

std::vector<std::string> list_img_files(int limit, std::string dataset_path);
std::vector<std::string> list_txt_files(int limit, std::string dataset_path);

#endif //VISEN_UTIL_H
