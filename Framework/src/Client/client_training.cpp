#include "client_training.h"

#include "definitions.h"

#include <map>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <random>

#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;

void extractHolidayFileNames(const char* base_path, int nImgs, std::map<int, std::string> &imgs) {
    DIR* dir;
    struct dirent* ent;
    int i = 0;
    if ((dir = opendir(base_path)) != NULL) {
        while ((ent = readdir(dir)) != NULL && i < nImgs) {
            std::string fname = ent->d_name;
            const size_t pos = fname.find(".jpg");
            if (pos != std::string::npos) {
                std::string idString = fname.substr(0, pos);
                const int id = atoi(idString.c_str());
                std::string path = base_path;
                path += fname;
                imgs[id] = path;
                i++;
            }
        }
        closedir(dir);
    } else {
        printf("Util::processHolidayDataset couldn't open dir");
        exit(1);
    }
}

float* client_train(const char* path, const unsigned nr_clusters, const unsigned desc_len, Ptr<SIFT> descriptor) {
    map<int, string> imgs;
    extractHolidayFileNames(path, 50000, imgs);

    //Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce");
    //BOWImgDescriptorExtractor* bowExtractor = new BOWImgDescriptorExtractor(surf, matcher);

    //timespec start = getTime();                         //getTime
    TermCriteria terminate_criterion;
    terminate_criterion.epsilon = FLT_EPSILON;
    BOWKMeansTrainer bowTrainer(nr_clusters, terminate_criterion, 3, KMEANS_PP_CENTERS);

    //std::srand(std::time(nullptr)); // use current time as seed for random generator
    //int random_variable = std::rand();
    for (map<int, string>::iterator it = imgs.begin(); it != imgs.end(); ++it) {
        const char* str = it->second.c_str();
        //const size_t len = strlen(str);

        /*if(!(str[len-5] == '0' && str[len-6] == '0'))
            continue;*/

        printf("train %s\n", str);
        Mat image = imread(it->second);

        vector<KeyPoint> keypoints;
        descriptor->detect(image, keypoints);

        Mat descriptors;
        descriptor->compute(image, keypoints, descriptors);

        bowTrainer.add(descriptors);
    }
    printf("build codebook with %d descriptors!\n", bowTrainer.descriptorsCount());
    Mat codebook = bowTrainer.cluster();
    //bowExtractor->setVocabulary(codebook);
    //bowExtractor->compute()
    //trainTime += diffSec(start, getTime());         //getTime
    //printf("Training Time: %f\n",trainTime);

    //const size_t desc_len = (size_t)codebook.size().width;
    const size_t nr_desc = (size_t)codebook.size().height;

    cout << "desc_len " << desc_len << endl;
    cout << "nr_desc " << nr_desc << endl;

    float* centroids = (float*)malloc(nr_desc * desc_len * sizeof(float));
    for (unsigned i = 0; i < nr_desc; i++) {
        for (size_t j = 0; j < desc_len; j++)
            centroids[i * desc_len + j] = *codebook.ptr<float>(i, j);
    }

    return centroids;
}
