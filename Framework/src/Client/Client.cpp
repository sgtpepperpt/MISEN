#include "Client.h"

#include "untrusted_util.h"
#include <getopt.h>
#include <sys/time.h>

using namespace cv;
using namespace cv::xfeatures2d;
using namespace std;

// TODO now hardcoded for dev
uint8_t key[AES_BLOCK_SIZE];
uint8_t ctr[AES_BLOCK_SIZE];

void debug_printbuf(uint8_t* buf, size_t len) {
    for (size_t l = 0; l < len; ++l)
        printf("%02x ", buf[l]);
    printf("\n");
}

vector<string> get_filenames(int n) {
    string holidayDir = "/home/guilherme/Datasets/inria";
    vector<string> filenames;

    DIR* dir;
    struct dirent* ent;
    int i = 0;
    if ((dir = opendir(holidayDir.c_str()))) {
        while ((ent = readdir(dir)) != NULL && i < n) {
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
    untrusted_util::socket_send(socket, &in_len, sizeof(size_t));
    untrusted_util::socket_send(socket, in, in_len);
}


void iee_recv(const int socket, uint8_t** out, size_t* out_len) {
    untrusted_util::socket_receive(socket, out_len, sizeof(size_t));
    *out = (uint8_t*)malloc(*out_len);
    untrusted_util::socket_receive(socket, *out, *out_len);
}

void iee_comm(const int socket, const void* in, const size_t in_len) {
    // encrypt before sending
    /*uint8_t* enc = (uint8_t*)malloc(in_len);
    memset(ctr, 0x00, AES_BLOCK_SIZE); // TODO do not repeat ctr
    c_encrypt(enc, (uint8_t *)in, in_len, key, ctr);

    debug_printbuf((uint8_t *)in, in_len);

    printf("query: %lu bytes; ", in_len);
    socket_send(socket, &in_len, sizeof(size_t));
    socket_send(socket, enc, in_len);

    // receive response (and discard for now) TODO do not
    size_t res_len;
    socket_receive(socket, &res_len, sizeof(size_t));
    printf("res: %lu bytes\n", res_len);

    unsigned char res[res_len];
    socket_receive(socket, res, res_len);

    uint8_t* dec = (uint8_t*)malloc(res_len);
    memset(ctr, 0x00, AES_BLOCK_SIZE); // TODO do not repeat ctr
    c_decrypt(dec, res, res_len, key, ctr);

//    debug_printbuf(dec, res_len);

    free(enc);
    free(dec);*/

    iee_send(socket, (uint8_t*)in, in_len);

    size_t res_len;
    uint8_t* res;
    iee_recv(socket, &res, &res_len);

    printf("res: %lu bytes\n", res_len);

    //debug_printbuf(res, res_len);
}

void init(uint8_t** in, size_t* in_len, unsigned nr_clusters, size_t row_len) {
    *in_len = sizeof(unsigned char) + sizeof(unsigned) + sizeof(size_t);
    *in = (uint8_t*)malloc(*in_len);

    *in[0] = 'i';
    memcpy(*in + sizeof(unsigned char), &nr_clusters, sizeof(unsigned));
    memcpy(*in + sizeof(unsigned char) + sizeof(unsigned), &row_len, sizeof(size_t));
}

void add_train_images(uint8_t** in, size_t* in_len, const Ptr<SURF> surf, std::string file_name) {
    const char* id = strrchr(file_name.c_str(), '/') + sizeof(char); // +sizeof(char) excludes the slash
    unsigned long num_id = strtoul(id, NULL, 0);

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

    tmp[0] = 'a';
    tmp += sizeof(unsigned char);

    memcpy(tmp, &num_id, sizeof(unsigned long));
    tmp += sizeof(unsigned long);

    memcpy(tmp, &nr_desc, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, descriptors_buffer, nr_desc * desc_len * sizeof(float));

    free(descriptors_buffer);
}

void add_images(uint8_t** in, size_t* in_len, const Ptr<SURF> surf, std::string file_name) {
    const char* id = strrchr(file_name.c_str(), '/') + sizeof(char); // +sizeof(char) excludes the slash
    unsigned long num_id = strtoul(id, NULL, 0);

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

    tmp[0] = 'n';
    tmp += sizeof(unsigned char);

    memcpy(tmp, &num_id, sizeof(unsigned long));
    tmp += sizeof(unsigned long);

    memcpy(tmp, &nr_desc, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, descriptors_buffer, nr_desc * desc_len * sizeof(float));

    free(descriptors_buffer);
}

void train(uint8_t** in, size_t* in_len) {
    *in_len = sizeof(unsigned char);

    *in = (uint8_t*)malloc(*in_len);
    *in[0] = 'k';
}

void clear(uint8_t** in, size_t* in_len) {
    *in_len = sizeof(unsigned char);

    *in = (uint8_t*)malloc(*in_len);
    *in[0] = 'c';
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

    tmp[0] = 's';
    tmp += sizeof(unsigned char);

    memcpy(tmp, &nr_desc, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, descriptors_buffer, nr_desc * desc_len * sizeof(float));

    free(descriptors_buffer);
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
    int src_img = 0;

    // parse terminal arguments
    int c;
    while ((c = getopt(argc, argv, "c:i:")) != -1) {
        switch (c) {
            case 'c':
                nr_clusters = stoul(optarg);
                break;
            case 'i':
                src_img = atoi(optarg);
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

    if (!argv[optind]) {
        printf("Nr of images not specified!\n");
        exit(1);
    }

    const int nr_images = atoi(argv[optind]);

    printf("Running with %d images, %lu clusters\n", nr_images, nr_clusters);

    struct timeval start;
    gettimeofday(&start, NULL);

    // establish connection to iee_comm server
    const int socket = untrusted_util::socket_connect(server_name, server_port);

    // image descriptor parameters
    Ptr<SURF> surf = SURF::create(surf_threshold);
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("FlannBased");
    Ptr<BOWImgDescriptorExtractor> bowExtractor = new BOWImgDescriptorExtractor(surf, matcher);

    // init iee and server
    size_t in_len = 0;
    uint8_t* in = NULL;

    init(&in, &in_len, nr_clusters, desc_len);
    iee_comm(socket, in, in_len);
    free(in);

    // adding
    const vector<string> files = get_filenames(nr_images);
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

    // add images to repository
    for (unsigned i = 0; i < files.size(); i++) {
        printf("Add img (%u/%lu)\n\n", i, files.size());
        add_images(&in, &in_len, surf, files[i]);
        iee_comm(socket, in, in_len);
        free(in);
    }

    // search
    search(&in, &in_len, surf, get_filenames(10)[src_img]);
    iee_send(socket, in, in_len);
    free(in);

    // receive response and print
    size_t res_len;
    uint8_t* res;
    iee_recv(socket, &res, &res_len);

    const size_t single_res_size = sizeof(unsigned long) + sizeof(double);

    unsigned nr;
    memcpy(&nr, res, sizeof(unsigned));
    printf("nr of imgs: %u\n", nr);

    for (unsigned j = 0; j < nr; ++j) {
        unsigned long id;
        memcpy(&id, res + sizeof(size_t) + j * single_res_size, sizeof(unsigned long));

        double score;
        memcpy(&score, res + sizeof(size_t) + j * single_res_size + sizeof(unsigned long), sizeof(double));

        printf("%lu %f\n", id, score);
    }

    free(res);

    // clear
    clear(&in, &in_len);
    iee_comm(socket, in, in_len);
    free(in);

    struct timeval end;
    gettimeofday(&end, NULL);
    printf("Total elapsed time: %ldms\n", untrusted_util::time_elapsed(start, end));

    return 0;
}
