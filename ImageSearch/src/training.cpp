#include "training.h"

// DEBUG nr of kmeans done
int ccount = 0;

void train_add_image(BagOfWordsTrainer* k, unsigned long id, size_t nr_desc, const uint8_t* in, const size_t in_len) {
    // TODO also pass descriptors pointer from caller

    outside_util::printf("add: id %lu nr_descs %d\n", id, nr_desc);

    // store id and desc counter in map TODO store in server until kmeans done? so that client does not need to resend?
    /*std::string i_str(id);
    image_descriptors[i_str] = nr_rows;*/

    // store in trainer
    img_descriptor* descriptor = (img_descriptor*)malloc(sizeof(img_descriptor));
    descriptor->count = nr_desc;

    descriptor->buffer = (float*)malloc(nr_desc * k->desc_len() * sizeof(float));
    memcpy(descriptor->buffer, in + sizeof(unsigned long) + sizeof(size_t), nr_desc * k->desc_len() * sizeof(float));

    k->add_descriptors(descriptor);

    if(k->is_full()) {
        outside_util::printf("Full, train images! %d\n", ccount++);
        k->cluster();
        //debug_print_points(centres->buffer, centres->count, k->desc_len());
    }
}

void train_kmeans(BagOfWordsTrainer* k) {
    k->cluster();
    //debug_print_points(centres->buffer, centres->count, k->desc_len());
}
