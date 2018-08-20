#include "util.h"

#include <dirent.h>
#include <algorithm>
#include <string.h>
#include <iostream>

using namespace std;

std::vector<std::string> list_img_files(int limit, string dataset_path) {
    vector<string> filenames;

    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(dataset_path.c_str()))) {
        while ((ent = readdir(dir)) != NULL) {
            std::string fname = dataset_path + ent->d_name;
            if (fname.find(".jpg") == string::npos || !fname.length())
                continue;

            filenames.push_back(fname);
        }
        closedir(dir);
    } else {
        printf("SseClient::listTxtFiles couldn't open dataset dir.\n");
        exit(1);
    }

    // remove elements in excess
    if(limit > 0 && filenames.size() > limit)
        filenames.erase(filenames.begin() + limit, filenames.end());

    // sort alphabetically
    sort(filenames.begin(), filenames.end());

    return filenames;
}

std::vector<std::string> list_txt_files(int limit, std::string path) {
    vector<string> filenames;

    DIR* dir;
    struct dirent* ent;
    if ((dir = opendir(path.c_str()))) {
        while ((ent = readdir(dir))) {
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..") || ent->d_name[0] == '.')
                continue;

            string fname = ent->d_name;
            const size_t pos = fname.find(".txt");
            if (pos != string::npos)
                filenames.push_back(path + fname);
        }
        closedir(dir);
    } else {
        printf("SseClient::listTxtFiles couldn't open dataset dir.\n");
        exit(1);
    }

    // remove elements in excess
    if(limit > 0 && filenames.size() > limit)
        filenames.erase(filenames.begin() + limit, filenames.end());

    // sort alphabetically
    sort(filenames.begin(), filenames.end());

    return filenames;
}
