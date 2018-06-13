#include "training.h"

#include <string.h>
#include <stdio.h>
#include "sgx_tprotected_fs.h"

#include "trusted_util.h"
#include "outside_util.h"
#include "util.h"
#include <vector>
#include <tuple>
#include <string>

typedef struct img_descriptor {
    unsigned count;
    float* descriptors;
} img_descriptor;

vector<img_descriptor*> descriptors;
unsigned file_pending = 0;
unsigned file_count = 0;

size_t descriptors_count = 0;
vector<tuple<std::string,int,int>> mapping;

void write_pending(BagOfWordsTrainer* k) {
    char fname[17];
    snprintf(fname, 17, "descriptors%05d", file_count);

    SGX_FILE* file = sgx_fopen_auto_key(fname, "wb");

    for(img_descriptor* desc : descriptors)
        sgx_fwrite(desc->descriptors, desc->count, k->get_desc_len(), file);

    sgx_fclose(file);

    std::string t = fname;
    mapping.push_back(std::make_tuple(t, descriptors_count - file_pending, descriptors_count));
    outside_util::printf("%s [%lu:%lu]\n", fname, descriptors_count - file_pending, descriptors_count);

    file_count++;
    file_pending = 0;
}

void train_add_image(BagOfWordsTrainer* k, unsigned long id, size_t nr_desc, const uint8_t* in, const size_t in_len) {
    // TODO also pass descriptors pointer from caller insted of BagOfWords

    outside_util::printf("add: id %lu nr_descs %lu\n", id, nr_desc);

    if(file_pending > 100000)
        write_pending(k);

    img_descriptor* descriptor = (img_descriptor*)malloc(sizeof(img_descriptor));
    descriptor->count = nr_desc;

    descriptor->descriptors = (float*)malloc(nr_desc * k->get_desc_len() * sizeof(float));
    memcpy(descriptor->descriptors, in + sizeof(unsigned long) + sizeof(size_t), nr_desc * k->get_desc_len() * sizeof(float));

    descriptors.push_back(descriptor);
    file_pending += nr_desc;
    descriptors_count += nr_desc;
}

void train_kmeans(BagOfWordsTrainer* k) {
    write_pending(k);

    const int nr_centroids = 1000;
    float* centroids = (float*)malloc(k->get_desc_len() * nr_centroids * sizeof(float));
    int nr_labels[nr_centroids];

    kmeans(mapping, descriptors_count, k->get_desc_len(), nr_centroids, nr_labels, 3, centroids);

    //debug_print_points(centres->buffer, centres->count, k->desc_len());
}

void train_kmeans_load(BagOfWordsTrainer* k) {

}
