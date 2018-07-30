#include "training.h"

#include <string.h>

#include "trusted_util.h"
#include "util.h"

// DEBUG nr of kmeans done
int ccount = 0;

void train_add_image(BagOfWordsTrainer* k, unsigned long id, size_t nr_desc, const uint8_t* in, const size_t in_len) {
    // TODO also pass descriptors pointer from caller insted of BagOfWords

    outside_util::printf("add: id %lu nr_descs %d\n", id, nr_desc);

    if(k->is_full_after(nr_desc)) {
        outside_util::printf("Full, train images! %d\n", ccount++);
        untrusted_time start = outside_util::curr_time();
        k->cluster();
        untrusted_time end = outside_util::curr_time();
        outside_util::printf("kmeans elapsed: %ld\n", trusted_util::time_elapsed_ms(start, end));
        //debug_print_points(centres->buffer, centres->count, k->desc_len());
    }

    // store id and desc counter in map TODO store in server until kmeans done? so that client does not need to resend?
    /*std::string i_str(id);
    image_descriptors[i_str] = nr_rows;*/

    // store in trainer
    img_descriptor* descriptor = (img_descriptor*)malloc(sizeof(img_descriptor));
    descriptor->count = nr_desc;

    descriptor->descriptors = (float*)malloc(nr_desc * k->get_desc_len() * sizeof(float));
    memcpy(descriptor->descriptors, in + sizeof(unsigned long) + sizeof(size_t), nr_desc * k->get_desc_len() * sizeof(float));

    k->add_descriptors(descriptor);
}

void train_kmeans(BagOfWordsTrainer* k) {
    k->cluster();
    //debug_print_points(centres->buffer, centres->count, k->desc_len());

    // debug: save to file
    void* file = trusted_util::open_secure("centres", "w");
    //outside_util::printf("file des %p\n", file);
    trusted_util::write_secure(k->get_all_centres(), k->nr_centres(), sizeof(float) * k->get_desc_len(), file);
    trusted_util::close_secure(file);
}

void train_kmeans_load(BagOfWordsTrainer* k) {
    // for debugging purposes for now
    // assumes current repository config was the same when the file was created (same desc_len and k)
    void* file = trusted_util::open_secure("centres", "r");
    void* centres = malloc(k->nr_centres() * sizeof(float) * k->get_desc_len());
    trusted_util::read_secure(centres, k->nr_centres(), sizeof(float) * k->get_desc_len(), file);
    trusted_util::close_secure(file);

    k->set_centres(centres);
}

void train_kmeans_set(BagOfWordsTrainer* k, float* centres, size_t nr_centres) {
    k->set_centres(centres);
}

#if 0
vector<img_descriptor*> descriptors;
unsigned file_pending = 0;

size_t descriptors_count = 0;
//vector<tuple<std::string,int,int>> mapping;

void write_pending(BagOfWordsTrainer* k) {
    for(img_descriptor* desc : descriptors) {
        outside_util::printf("store desc %u\n", desc->count);

        //for (unsigned i = 0; i < desc->count; ++i) {
            /*int to_send = 1;
            const size_t size = sizeof(unsigned char) + sizeof(unsigned) + to_send * 64 * sizeof(float);
            outside_util::uee_send(socket, &size, sizeof(size_t));
            const uint8_t op = '5';
            outside_util::uee_send(socket, &op, sizeof(unsigned char));
            outside_util::uee_send(socket, &to_send, sizeof(unsigned));
            //outside_util::printf("sending buffer %lu %u %lu\n", to_send * 64 * sizeof(float), to_send, sizeof(float));
            outside_util::uee_send(socket, desc->descriptors, to_send * 64 * sizeof(float));*/

outside_util::set(desc->count, desc->descriptors);
//}

//uint8_t* res;
//size_t res_len;
//outside_util::uee_process(socket, (void**)&res, &res_len, desc->descriptors, desc->count * 64 * sizeof(float));

free(desc->descriptors);
free(desc);
}
descriptors.clear();
/*
std::string t = fname;
mapping.push_back(std::make_tuple(t, descriptors_count - file_pending, descriptors_count));
outside_util::printf("%s [%lu:%lu]\n", fname, descriptors_count - file_pending, descriptors_count);
*/
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
    k->store(centroids);
    int nr_labels[nr_centroids];

    train_kmeans(descriptors_count, k->get_desc_len(), nr_centroids, nr_labels, 3, centroids);

    //debug_print_points(centres->buffer, centres->count, k->desc_len());
}
#endif
