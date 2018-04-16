#include "training.h"

// DEBUG nr of kmeans done
int ccount = 0;

void train_add_image(BagOfWordsTrainer* k, void** out, size_t* out_len, const uint8_t* in, const size_t in_len) {
    unsigned long id;
    size_t nr_rows;

    memcpy(&id, in, sizeof(unsigned long));
    memcpy(&nr_rows, in + sizeof(unsigned long), sizeof(size_t));

    sgx_printf("add: id %lu nr_descs %d\n", id, nr_rows);

    // store id and desc counter in map TODO store in server until kmeans done? so that client does not need to resend?
    /*std::string i_str(id);
    image_descriptors[i_str] = nr_rows;*/

    // store in trainer
    img_descriptor* descriptor = (img_descriptor*)malloc(sizeof(img_descriptor));
    descriptor->count = nr_rows;

    descriptor->buffer = (float*)malloc(nr_rows * k->desc_len() * sizeof(float));
    memcpy(descriptor->buffer, in + sizeof(unsigned long) + sizeof(size_t), nr_rows * k->desc_len() * sizeof(float));

    k->add_descriptors(descriptor);

    if(k->is_full()) {
        sgx_printf("Full, train images! %d\n", ccount++);
        img_descriptor* centres = k->cluster();
        //debug_print_points(centres->buffer, centres->count, k->desc_len());
        free(centres);
    }

    // return ok to client
    ok_response(out, out_len);
}

void train_kmeans(BagOfWordsTrainer* k, void** out, size_t* out_len, const uint8_t* in, const size_t in_len) {
    img_descriptor* centres = k->cluster();
    //debug_print_points(centres->buffer, centres->count, k->desc_len());
    free(centres);

    // return ok
    ok_response(out, out_len);
}
