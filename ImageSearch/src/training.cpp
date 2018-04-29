#include "training.h"

#include <string.h>
#include "sgx_tprotected_fs.h"

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

    descriptor->buffer = (float*)malloc(nr_desc * k->desc_len() * sizeof(float));
    memcpy(descriptor->buffer, in + sizeof(unsigned long) + sizeof(size_t), nr_desc * k->desc_len() * sizeof(float));

    k->add_descriptors(descriptor);
}

void train_kmeans(BagOfWordsTrainer* k) {
    k->cluster();
    //debug_print_points(centres->buffer, centres->count, k->desc_len());

    // debug: save to file
    SGX_FILE* file = sgx_fopen_auto_key("centres", "w");
    //outside_util::printf("file des %p\n", file);
    sgx_fwrite(k->get_all_centres(), k->nr_centres(), sizeof(float) * k->desc_len(), file);
    sgx_fclose(file);
}

void train_kmeans_load(BagOfWordsTrainer* k) {
    // for debugging purposes for now
    // assumes current repository config was the same when the file was created (same desc_len and k)
    SGX_FILE* file = sgx_fopen_auto_key("centres", "r");
    void* centres = malloc(k->nr_centres() * sizeof(float) * k->desc_len());
    sgx_fread(centres, k->nr_centres(), sizeof(float) * k->desc_len(), file);
    sgx_fclose(file);

    k->set_centres(centres);
}
