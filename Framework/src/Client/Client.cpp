#include "Client.h"

#include "untrusted_util.h"
#include <getopt.h>
#include <fstream>
#include <sys/time.h>
#include <sodium.h>

using namespace cv;
using namespace cv::xfeatures2d;
using namespace std;

// TODO now hardcoded for dev
uint8_t key[AES_BLOCK_SIZE];
uint8_t ctr[AES_BLOCK_SIZE];


#define STORE_RESULTS 1

#define SRC_RES_LEN (sizeof(unsigned long) + sizeof(double))

#if STORE_RESULTS

void printHolidayResults(const char* path, std::map<unsigned long, std::vector<unsigned long>> results) {
    std::ofstream ofs(path);
    std::map<unsigned long, std::vector<unsigned long>>::iterator it;
    for (it = results.begin(); it != results.end(); ++it) {
        ofs << it->first << ".jpg";
        for (unsigned long i = 0; i < it->second.size(); i++)
            ofs << " " << i << " " << it->second.at(i) << ".jpg";
        ofs << std::endl;
    }

    ofs.close();
}

#endif

unsigned long filename_to_id(const char* filename) {
    const char* id = strrchr(filename, '/') + sizeof(char); // +sizeof(char) excludes the slash
    return strtoul(id, NULL, 0);
}

vector<string> get_filenames(int n) {
    string holidayDir = "/home/guilherme/Datasets/inria";
    vector<string> filenames;

    DIR* dir;
    struct dirent* ent;
    int i = 0;
    if ((dir = opendir(holidayDir.c_str()))) {
        while ((ent = readdir(dir)) != NULL && (n < 0 || i < n)) {
            std::string fname = holidayDir + "/" + ent->d_name;
            if (fname.find(".jpg") == string::npos || !fname.length())
                continue;

            //printf("%s\n", fname.c_str());
            filenames.push_back(fname);
            i++;
        }
        closedir(dir);
    }

    return filenames;
}

void iee_send(const int socket, const uint8_t* in, const size_t in_len) {
    // TODO do not use these hardcoded values
    uint8_t key[crypto_secretbox_KEYBYTES];
    memset(key, 0x00, crypto_secretbox_KEYBYTES);

    uint8_t nonce[crypto_secretbox_NONCEBYTES];
    memset(nonce, 0x00, crypto_secretbox_NONCEBYTES);

    // encrypt before sending
    size_t enc_len = in_len + SODIUM_EXPBYTES;
    uint8_t* enc = (uint8_t*)malloc(enc_len);
    utcrypto::sodium_encrypt(enc, (uint8_t *)in, in_len, nonce, key);

    untrusted_util::socket_send(socket, &enc_len, sizeof(size_t));
    untrusted_util::socket_send(socket, enc, enc_len);

    free(enc);
}

void iee_recv(const int socket, uint8_t** out, size_t* out_len) {
    // TODO do not use these hardcoded values
    uint8_t key[crypto_secretbox_KEYBYTES];
    memset(key, 0x00, crypto_secretbox_KEYBYTES);

    uint8_t nonce[crypto_secretbox_NONCEBYTES];
    memset(nonce, 0x00, crypto_secretbox_NONCEBYTES);

    size_t res_len;
    unsigned char* res;
    untrusted_util::socket_receive(socket, &res_len, sizeof(size_t));
    res = (uint8_t*)malloc(res_len);
    untrusted_util::socket_receive(socket, res, res_len);

    *out_len = res_len - SODIUM_EXPBYTES;
    *out = (uint8_t*)malloc(*out_len);
    utcrypto::sodium_decrypt(*out, res, res_len, nonce, key);

    free(res);
}

void iee_comm(const int socket, const void* in, const size_t in_len) {
    iee_send(socket, (uint8_t*)in, in_len);

    size_t res_len;
    unsigned char* res;
    iee_recv(socket, &res, &res_len);

    printf("res: %lu bytes\n", res_len);
    free(res);
}

void init(uint8_t** in, size_t* in_len, unsigned nr_clusters, size_t row_len) {
    *in_len = sizeof(unsigned char) + sizeof(unsigned) + sizeof(size_t);
    *in = (uint8_t*)malloc(*in_len);

    *in[0] = OP_IEE_INIT;
    memcpy(*in + sizeof(unsigned char), &nr_clusters, sizeof(unsigned));
    memcpy(*in + sizeof(unsigned char) + sizeof(unsigned), &row_len, sizeof(size_t));
}

void add_train_images(uint8_t** in, size_t* in_len, const Ptr<SURF> surf, std::string file_name) {
    unsigned long id = filename_to_id(file_name.c_str());

    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    surf->detect(image, keypoints);

    Mat descriptors;
    surf->compute(image, keypoints, descriptors);

    const size_t desc_len = (size_t)descriptors.size().width;
    const size_t nr_desc = (size_t)descriptors.size().height;

    float* descriptors_buffer = (float*)malloc(desc_len * nr_desc * sizeof(float));
    for (unsigned i = 0; i < nr_desc; i++) {
        for (size_t j = 0; j < desc_len; j++)
            descriptors_buffer[i * desc_len + j] = *descriptors.ptr<float>(i, j);
    }

    // send
    *in_len = sizeof(unsigned char) + sizeof(unsigned long) + sizeof(size_t) + nr_desc * desc_len * sizeof(float);
    *in = (uint8_t*)malloc(*in_len);
    uint8_t* tmp = *in;

    tmp[0] = OP_IEE_TRAIN_ADD;
    tmp += sizeof(unsigned char);

    memcpy(tmp, &id, sizeof(unsigned long));
    tmp += sizeof(unsigned long);

    memcpy(tmp, &nr_desc, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, descriptors_buffer, nr_desc * desc_len * sizeof(float));

    free(descriptors_buffer);
}

void add_images(uint8_t** in, size_t* in_len, const Ptr<SURF> surf, std::string file_name) {
    unsigned long id = filename_to_id(file_name.c_str());

    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    surf->detect(image, keypoints);

    Mat descriptors;
    surf->compute(image, keypoints, descriptors);

    const size_t desc_len = (size_t)descriptors.size().width;
    const size_t nr_desc = (size_t)descriptors.size().height;

    float* descriptors_buffer = (float*)malloc(desc_len * nr_desc * sizeof(float));
    for (unsigned i = 0; i < nr_desc; i++) {
        for (size_t j = 0; j < desc_len; j++)
            descriptors_buffer[i * desc_len + j] = *descriptors.ptr<float>(i, j);
    }

    // send
    *in_len = sizeof(unsigned char) + sizeof(unsigned long) + sizeof(size_t) + nr_desc * desc_len * sizeof(float);
    *in = (uint8_t*)malloc(*in_len);
    uint8_t* tmp = *in;

    tmp[0] = OP_IEE_ADD;
    tmp += sizeof(unsigned char);

    memcpy(tmp, &id, sizeof(unsigned long));
    tmp += sizeof(unsigned long);

    memcpy(tmp, &nr_desc, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, descriptors_buffer, nr_desc * desc_len * sizeof(float));

    free(descriptors_buffer);
}

void train(uint8_t** in, size_t* in_len) {
    *in_len = sizeof(unsigned char);

    *in = (uint8_t*)malloc(*in_len);
    *in[0] = OP_IEE_TRAIN;
}

void train_load_clusters(uint8_t** in, size_t* in_len) {
    *in_len = sizeof(unsigned char);

    *in = (uint8_t*)malloc(*in_len);
    *in[0] = OP_IEE_LOAD;
}

void clear(uint8_t** in, size_t* in_len) {
    *in_len = sizeof(unsigned char);

    *in = (uint8_t*)malloc(*in_len);
    *in[0] = OP_IEE_CLEAR;
}

void search(uint8_t** in, size_t* in_len, const Ptr<SURF> surf, const std::string file_name) {
    const char* id = strrchr(file_name.c_str(), '/') + sizeof(char); // +sizeof(char) excludes the slash
    printf(" - Search for %s -\n", id);

    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    surf->detect(image, keypoints);

    Mat descriptors;
    surf->compute(image, keypoints, descriptors);

    const size_t desc_len = (size_t)descriptors.size().width;
    const size_t nr_desc = (size_t)descriptors.size().height;

    float* descriptors_buffer = (float*)malloc(desc_len * nr_desc * sizeof(float));

    for (unsigned i = 0; i < nr_desc; ++i) {
        for (size_t j = 0; j < desc_len; ++j)
            descriptors_buffer[i * desc_len + j] = *descriptors.ptr<float>(i, j);
    }

    // send
    *in_len = sizeof(unsigned char) + sizeof(size_t) + nr_desc * desc_len * sizeof(float);

    *in = (uint8_t*)malloc(*in_len);
    uint8_t* tmp = *in;

    tmp[0] = OP_IEE_SEARCH;
    tmp += sizeof(unsigned char);

    memcpy(tmp, &nr_desc, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, descriptors_buffer, nr_desc * desc_len * sizeof(float));

    free(descriptors_buffer);
}

void search_test(const int socket, const Ptr<SURF> surf) {
    size_t in_len;
    uint8_t* in;

    map<unsigned long, vector<unsigned long>> precision_res;

    // generate file list to search
    vector<string> files;
    for (int i = 100000; i <= 149900; i += 100) {
        char img[128];
        sprintf(img, "/home/guilherme/Datasets/inria/%d.jpg", i);
        files.push_back(img);
    }

    for (size_t i = 0; i < files.size(); ++i) {
        search(&in, &in_len, surf, files[i]);
        iee_send(socket, in, in_len);
        free(in);

        // receive response
        size_t res_len;
        uint8_t* res;
        iee_recv(socket, &res, &res_len);

        // get number of response docs
        unsigned nr;
        memcpy(&nr, res, sizeof(unsigned));
        printf("nr of imgs: %u\n", nr);

#if STORE_RESULTS
        // store the results in order
        vector<unsigned long> precision_results;
#endif

        // decode id and score for each doc
        for (unsigned j = 0; j < nr; ++j) {
            unsigned long id;
            memcpy(&id, res + sizeof(size_t) + j * SRC_RES_LEN, sizeof(unsigned long));

            double score;
            memcpy(&score, res + sizeof(size_t) + j * SRC_RES_LEN + sizeof(unsigned long), sizeof(double));

            printf("%lu %f\n", id, score);
#if STORE_RESULTS
            precision_results.push_back(id);
#endif
        }

        free(res);
        precision_res[filename_to_id(files[i].c_str())] = precision_results;
    }

#if STORE_RESULTS
    // write results to file
    printHolidayResults("results.dat", precision_res);
#endif
}

int main(int argc, char** argv) {
    memset(key, 0x00, AES_BLOCK_SIZE);
    memset(ctr, 0x00, AES_BLOCK_SIZE);

    // fixed params
    const char* server_name = IEE_HOSTNAME;
    const int server_port = IEE_PORT;
    const size_t desc_len = 64;
    const double surf_threshold = 400;

    // may be specified by user
    size_t nr_clusters = 1000;
    int train_only = 0;
    int load_clusters = 0;
    int do_search_test = 0;
    int add_all = 0;

    int load_uee = 0;
    int write_uee = 0;

    // parse terminal arguments
    int c;
    while ((c = getopt(argc, argv, "htlsarwk:")) != -1) {
        switch (c) {
            case 'k':
                nr_clusters = stoul(optarg);
                break;
            case 't':
                train_only = 1;
                break;
            case 'h':
                printf("Usage: ./Client nr-imgs\n");
                exit(0);
            case 'l':
                load_clusters = 1;
                break;
            case 's':
                do_search_test = 1;
                break;
            case 'a':
                add_all = 1;
                break;
            case 'r':
                load_uee = 1;
                break;
            case 'w':
                write_uee = 1;
                break;
            case '?':
                if (optopt == 'c')
                    fprintf(stderr, "-%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                exit(1);
            default:
                exit(-1);
        }
    }

    if (argv[optind] && (add_all || load_uee)) {
        printf("Nr of images can not be specified with add all or load uee!\n");
        exit(1);
    }

    if (!argv[optind] && !load_uee) {
        printf("Nr of images not specified!\n");
        exit(1);
    }

    if (load_uee && add_all) {
        printf("Add all images is not compatible with load uee option!\n");
        exit(1);
    }

    if (load_uee && write_uee) {
        printf("Load UEE is not compatible with write UEE option!\n");
        exit(1);
    }

    const int nr_images = add_all || load_uee ? -1 : atoi(argv[optind]);

    printf("Running with %d images, %lu clusters\n", nr_images, nr_clusters);

    struct timeval start;
    gettimeofday(&start, NULL);

    // establish connection to iee_comm server
    const int socket = untrusted_util::socket_connect(server_name, server_port);

    // image descriptor parameters
    Ptr<SURF> surf = SURF::create(surf_threshold);
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("FlannBased");
    //BOWImgDescriptorExtractor* bowExtractor = new BOWImgDescriptorExtractor(surf, matcher);

    const vector<string> files = get_filenames(nr_images);

    // init iee and server
    size_t in_len = 0;
    uint8_t* in = NULL;

    init(&in, &in_len, nr_clusters, desc_len);
    iee_comm(socket, in, in_len);
    free(in);

    if (!load_clusters) {
        // adding
        for (unsigned i = 0; i < files.size(); i++) {
            printf("Train img (%u/%lu) %s\n", i, files.size(), files[i].c_str());
            add_train_images(&in, &in_len, surf, files[i]);
            iee_comm(socket, in, in_len);
            free(in);
            printf("\n");
        }

        // train
        train(&in, &in_len);
        iee_comm(socket, in, in_len);
        free(in);
    } else {
        train_load_clusters(&in, &in_len);
        iee_comm(socket, in, in_len);
        free(in);
    }

    struct timeval end;
    gettimeofday(&end, NULL);
    printf("-- Train elapsed time: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

    if (train_only)
        return 0;

    if (load_uee) {
        // tell iee to load images from disc
        unsigned char op = OP_IEE_READ_MAP;
        iee_comm(socket, &op, 1);
    } else {
        // add images to repository
        for (unsigned i = 0; i < files.size(); i++) {
            printf("Add img (%u/%lu)\n\n", i, files.size());
            add_images(&in, &in_len, surf, files[i]);
            iee_comm(socket, in, in_len);
            free(in);
        }
    }

    if (write_uee) {
        // send persist message to iee
        unsigned char op = OP_IEE_WRITE_MAP;
        iee_comm(socket, &op, 1);
    }

    // search
    if (do_search_test) {
        search_test(socket, surf);
    } else {
        //TODO perform a search from args?
        search(&in, &in_len, surf, get_filenames(10)[0]);
        iee_send(socket, in, in_len);
        free(in);

        // receive response and print
        size_t res_len;
        uint8_t* res;
        iee_recv(socket, &res, &res_len);

        unsigned nr;
        memcpy(&nr, res, sizeof(unsigned));
        printf("nr of imgs: %u\n", nr);

        for (unsigned j = 0; j < nr; ++j) {
            unsigned long id;
            memcpy(&id, res + sizeof(size_t) + j * SRC_RES_LEN, sizeof(unsigned long));

            double score;
            memcpy(&score, res + sizeof(size_t) + j * SRC_RES_LEN + sizeof(unsigned long), sizeof(double));

            printf("%lu %f\n", id, score);

        }
    }


    // clear
    clear(&in, &in_len);
    iee_comm(socket, in, in_len);
    free(in);

    gettimeofday(&end, NULL);
    printf("Total elapsed time: %ldms\n", untrusted_util::time_elapsed_ms(start, end));

    return 0;
}
