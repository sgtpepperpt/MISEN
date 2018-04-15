#include "Client.h"

using namespace cv;
using namespace cv::xfeatures2d;
using namespace std;

vector<string> get_filenames(int n) {
    string holidayDir = "/home/guilherme/Datasets/inria";
    vector<string> filenames;

    DIR *dir;
    struct dirent *ent;
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

void iee_comm(const int socket, const void *in, const size_t in_len) {
    printf("Iee query; %lu bytes\n", in_len);
    socket_send(socket, &in_len, sizeof(size_t));
    socket_send(socket, in, in_len);

    // receive response (and discard for now) TODO
    size_t res_len;
    socket_receive(socket, &res_len, sizeof(size_t));
    printf("Iee res: %lu bytes\n", res_len);
    unsigned char res[res_len];
    socket_receive(socket, res, res_len);


    for (int i = 0; i < res_len; ++i)
        printf("%c ", res[i]);
    printf("\n");
}

void init(uint8_t **in, size_t *in_len, unsigned nr_clusters, size_t row_len) {
    *in_len = sizeof(unsigned char) + sizeof(unsigned) + sizeof(size_t);
    *in = (uint8_t *) malloc(*in_len);

    *in[0] = 'i';
    memcpy(*in + sizeof(unsigned char), &nr_clusters, sizeof(unsigned));
    memcpy(*in + sizeof(unsigned char) + sizeof(unsigned), &row_len, sizeof(size_t));
}

void add_train_images(uint8_t **in, size_t *in_len, const Ptr<SURF> surf, std::string file_name) {
    const char *id = strrchr(file_name.c_str(), '/') + sizeof(char); // +sizeof(char) excludes the slash
    unsigned long num_id = strtoul(id, NULL, 0);
    printf("%s\n", id);

    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    surf->detect(image, keypoints);

    Mat descriptors;
    surf->compute(image, keypoints, descriptors);

    const size_t desc_len = (size_t) descriptors.size().width;
    const size_t nr_desc = (size_t) descriptors.size().height;

    float *descriptors_buffer = (float *) malloc(desc_len * nr_desc * sizeof(float));
    for (unsigned i = 0; i < nr_desc; i++) {
        for (int j = 0; j < desc_len; j++)
            descriptors_buffer[i * desc_len + j] = *descriptors.ptr<float>(i, j);
    }

    // send
    *in_len = sizeof(unsigned char) + sizeof(unsigned long) + sizeof(size_t) + nr_desc * desc_len * sizeof(float);
    *in = (uint8_t *) malloc(*in_len);
    uint8_t *tmp = *in;

    tmp[0] = 'a';
    tmp += sizeof(unsigned char);

    memcpy(tmp, &num_id, sizeof(unsigned long));
    tmp += sizeof(unsigned long);

    memcpy(tmp, &nr_desc, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, descriptors_buffer, nr_desc * desc_len * sizeof(float));

    free(descriptors_buffer);
}

void add_images(uint8_t **in, size_t *in_len, const Ptr<SURF> surf, std::string file_name) {
    const char *id = strrchr(file_name.c_str(), '/') + sizeof(char); // +sizeof(char) excludes the slash
    unsigned long num_id = strtoul(id, NULL, 0);
    printf("%s\n", id);

    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    surf->detect(image, keypoints);

    Mat descriptors;
    surf->compute(image, keypoints, descriptors);

    const size_t desc_len = (size_t) descriptors.size().width;
    const size_t nr_desc = (size_t) descriptors.size().height;

    float *descriptors_buffer = (float *) malloc(desc_len * nr_desc * sizeof(float));
    for (unsigned i = 0; i < nr_desc; i++) {
        for (int j = 0; j < desc_len; j++)
            descriptors_buffer[i * desc_len + j] = *descriptors.ptr<float>(i, j);
    }

    // send
    *in_len = sizeof(unsigned char) + sizeof(unsigned long) + sizeof(size_t) + nr_desc * desc_len * sizeof(float);
    *in = (uint8_t *) malloc(*in_len);
    uint8_t *tmp = *in;

    tmp[0] = 'n';
    tmp += sizeof(unsigned char);

    memcpy(tmp, &num_id, sizeof(unsigned long));
    tmp += sizeof(unsigned long);

    memcpy(tmp, &nr_desc, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, descriptors_buffer, nr_desc * desc_len * sizeof(float));

    free(descriptors_buffer);
}

void train(uint8_t **in, size_t *in_len) {
    *in_len = sizeof(unsigned char);

    *in = (uint8_t *) malloc(*in_len);
    *in[0] = 'k';
}

void clear(uint8_t **in, size_t *in_len) {
    *in_len = sizeof(unsigned char);

    *in = (uint8_t *) malloc(*in_len);
    *in[0] = 'c';
}

void search(uint8_t **in, size_t *in_len, const Ptr<SURF> surf, const std::string file_name) {
    const char *id = strrchr(file_name.c_str(), '/') + sizeof(char); // +sizeof(char) excludes the slash
    printf("Search for %s\n", id);

    Mat image = imread(file_name);
    if (!image.data) {
        printf("No image data for %s\n", file_name.c_str());
        exit(1);
    }

    vector<KeyPoint> keypoints;
    surf->detect(image, keypoints);

    Mat descriptors;
    surf->compute(image, keypoints, descriptors);

    const size_t row_len = (size_t) descriptors.size().width;
    const size_t nr_rows = (size_t) descriptors.size().height;

    float *descriptors_buffer = (float *) malloc(row_len * nr_rows * sizeof(float));

    for (unsigned i = 0; i < nr_rows; ++i) {
        for (int j = 0; j < row_len; ++j)
            descriptors_buffer[i * row_len + j] = *descriptors.ptr<float>(i, j);
    }

    // send
    *in_len = sizeof(unsigned char) + 2 * sizeof(size_t) + nr_rows * row_len * sizeof(float);

    *in = (uint8_t *) malloc(*in_len);
    uint8_t *tmp = *in;

    tmp[0] = 's';
    tmp += sizeof(unsigned char);

    memcpy(tmp, &nr_rows, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, &row_len, sizeof(size_t));
    tmp += sizeof(size_t);

    memcpy(tmp, descriptors_buffer, nr_rows * row_len * sizeof(float));

    free(descriptors_buffer);
}

int main(int argc, char **argv) {
    const char *server_name = "localhost";
    const int server_port = 7910;

    if (argc != 2) {
        printf("no arg\n");
        exit(1);
    }

    const size_t row_len = 64;
    const size_t nr_clusters = 100;
    const int nr_images = atoi(argv[1]);

    // parse terminal arguments
    /*int c;
    while ((c = getopt(argc, argv, "c:")) != -1) {
        switch (c) {
        case 'c':
            nr_clusters = stoul(optarg);
            break;
        case '?':
            if (optopt == 'c')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            return 1;
        default:
            abort();
        }
    }*/

    // establish connection to iee_comm server
    const int socket = socket_connect(server_name, server_port);

    // image descriptor parameters
    Ptr<SURF> surf = SURF::create(400);
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("FlannBased");
    Ptr<BOWImgDescriptorExtractor> bowExtractor = new BOWImgDescriptorExtractor(surf, matcher);

    // init iee and server
    size_t in_len = 0;
    uint8_t *in = NULL;

    init(&in, &in_len, nr_clusters, row_len);
    iee_comm(socket, in, in_len);
    free(in);

    // adding
    const vector<string> files = get_filenames(nr_images);
    for (unsigned i = 0; i < files.size(); i++) {
        add_train_images(&in, &in_len, surf, files[i]);
        iee_comm(socket, in, in_len);
        free(in);
    }

    // train
    train(&in, &in_len);
    iee_comm(socket, in, in_len);
    free(in);

    // add images to repository
    for (unsigned i = 0; i < files.size(); i++) {
        add_images(&in, &in_len, surf, files[i]);
        iee_comm(socket, in, in_len);
        free(in);
    }

    // search
    search(&in, &in_len, surf, get_filenames(10)[0]);
    iee_comm(socket, in, in_len);
    free(in);

    // clear
    clear(&in, &in_len);
    iee_comm(socket, in, in_len);
    free(in);

    return 0;
}
