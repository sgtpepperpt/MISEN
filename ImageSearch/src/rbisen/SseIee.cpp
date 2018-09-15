#include "SseIee.h"

#include <math.h>
#include <algorithm>
#include "outside_util.h"
#include "trusted_util.h"

#include "IeeUtils.h"
#include "QueryEvaluator.h"

#include "vec_int.h"
#include "vec_token.h"
#include "IeeCrypt.h"

#define MAX_BATCH_UPDATE 1000
#define MAX_BATCH_SEARCH 2000

static void init_pipes();
//static void destroy_pipes();
static void benchmarking_print(); // only for benchmarking, prints stats
static void setup(bytes* out, size* out_len, uint8_t* in, const size in_len);
static void update(bytes* out, size* out_len, uint8_t* in, const size in_len);
static void search(bytes* out, size* out_len, uint8_t* in, const size in_len);
static void get_docs_from_server(vec_token *query, const unsigned count_words, const unsigned total_labels);

static int server_socket;
static unsigned last_ndocs;
static unsigned char* aux_bool;

double total_iee_add = 0, total_server_add = 0;
double total_iee_search = 0, total_server_search = 0;
size_t count_add = 0, count_search = 0;

// IEE entry point
void f(bytes* out, size* out_len, const unsigned long long pid, uint8_t* in, const size in_len) {
    #ifdef VERBOSE
    //ocall_strprint("\n***** Entering IEE *****\n");
    #endif

    // set out variables
    *out = NULL;
    *out_len = 0;

    //setup operation
    if(in[0] == OP_SETUP)
        setup(out, out_len, in, in_len);
    //add / update operation
    else if (in[0] == OP_ADD)
        update(out, out_len, in, in_len);
    //search operation
    else if (in[0] == OP_SRC)
        search(out, out_len, in, in_len);
    else if (in[0] == OP_BENCH)
        benchmarking_print();

    #ifdef VERBOSE
    //ocall_strprint("\n***** Leaving IEE *****\n\n");
    #endif
}

void benchmarking_print() {
    // BENCHMARK : tell server to print statistics
    // this instruction can be safely removed if wanted
    unsigned char* tmp_buff = (unsigned char*)malloc(sizeof(unsigned char));
    tmp_buff[0] = '4';
    outside_util::write(server_socket, tmp_buff, sizeof(unsigned char));
}

void init_pipes() {
    server_socket = outside_util::open_socket("localhost", 7899);
    outside_util::printf("Finished IEE init! Gonna start listening for client requests through bridge!\n");
}

static void setup(bytes* out, size* out_len, uint8_t* in, const size in_len) {
#ifdef VERBOSE
    ocall_print_string("IEE: Starting Setup!\n");
#endif

    init_pipes();
    void* tmp = in + 1; // exclude op

    // read kEnc
    size_t kEnc_size;
    memcpy(&kEnc_size, tmp, sizeof(size_t));
    tmp = (char*)tmp + sizeof(size_t);

    unsigned char* kEnc = (unsigned char*)malloc(sizeof(unsigned char) * kEnc_size);
    memcpy(kEnc, tmp, kEnc_size);
    tmp = (char*)tmp + kEnc_size;

    // read kF
    size_t kF_size;
    memcpy(&kF_size, tmp, sizeof(size_t));
    tmp = (char*)tmp + sizeof(size_t);

    unsigned char* kF = (unsigned char*)malloc(sizeof(unsigned char) * kF_size);
    memcpy(kF, tmp, kF_size);
    //tmp = (char*)tmp + kF_size;

    /*printf("kEnc size %d\n", kEnc_size);
    printf("kF size %d\n", kF_size);*/

    // Keys are  given by the client (as input message)
    setKeys(kEnc, kF);

    // tell server to init index I
    unsigned char op = '1';
    outside_util::write(server_socket, &op, sizeof(char));

    // init aux_bool buffer
    aux_bool = NULL;
    last_ndocs = 0;

    // output message
    *out_len = 1;
    *out = (unsigned char*)malloc(sizeof(unsigned char));
    (*out)[0] = 0x90;

#ifdef VERBOSE
    ocall_print_string("IEE: Finished Setup!\n");
#endif
}

static void update(bytes *out, size *out_len, uint8_t* in, const size in_len) {
#ifdef VERBOSE
    //ocall_print_string("IEE: Started add!\n");
#endif

    untrusted_time start, end;
    start = outside_util::curr_time();

    // read buffer
    void* tmp = in + 1; // exclude op
    const size_t to_recv_len = in_len - sizeof(unsigned char);

    // generate nonce
    unsigned char* nonce = (unsigned char*)malloc(sizeof(unsigned char) * C_NONCESIZE);
    for(unsigned i = 0; i < C_NONCESIZE; i++)
        nonce[i] = 0x00;

    // size of single buffers
    const size_t recv_len = H_BYTES + 3 * sizeof(int);
    const size_t d_unenc_len = H_BYTES + 2 * sizeof(int);
    const size_t d_enc_len = d_unenc_len + C_EXPBYTES;

    // size of batch requests to server
    size_t to_recv = to_recv_len / recv_len;

    // op to server
    const unsigned char op = '2';

    // buffers for reuse while receiving
    unsigned char* unenc_data = (unsigned char*)malloc(sizeof(unsigned char) * d_unenc_len);
    unsigned char* kW = (unsigned char*)malloc(sizeof(unsigned char) * H_BYTES);

    // will contain at most max_batch_size (l,id*) pairs
    const size_t pair_size = H_BYTES + d_enc_len;
    void* batch_buffer = malloc(MAX_BATCH_UPDATE * pair_size);

    end = outside_util::curr_time();
    total_iee_add += trusted_util::time_elapsed_ms(start, end);

    while(to_recv) {
        start = outside_util::curr_time();
        memset(batch_buffer, 0, MAX_BATCH_UPDATE * (H_BYTES + d_enc_len)); // fix syscall param write(buf) points to uninitialised byte(s)

        // calculate size and get batch from client
        size_t batch_size = std::min(to_recv, (size_t)MAX_BATCH_UPDATE);
        for (unsigned i = 0; i < batch_size; i++) {
            void* label = batch_buffer + i * pair_size;
            void* d = label + H_BYTES;

            //get doc_id, counter, frequency, kW from array
            int doc_id;
            memcpy(&doc_id, tmp, sizeof(int));
            tmp = (char*)tmp + sizeof(int);

            int counter;
            memcpy(&counter, tmp, sizeof(int));
            tmp = (char*)tmp + sizeof(int);

            int frequency;
            memcpy(&frequency, tmp, sizeof(int));
            tmp = (char*)tmp + sizeof(int);

            // read kW
            memcpy(kW, tmp, H_BYTES);
            tmp = (char*)tmp + H_BYTES;

            // calculate "label" (key) and add to batch_buffer
            rbisen_c_hmac((unsigned char*)label, (unsigned char*)&counter, sizeof(int), kW);

            // calculate "id*" (entry)
            // hmac + frequency + doc_id
            memcpy(unenc_data, label, H_BYTES);
            memcpy(unenc_data + H_BYTES, &frequency, sizeof(int));
            memcpy(unenc_data + H_BYTES + sizeof(int), &doc_id, sizeof(int));

            // store in batch_buffer
            rbisen_c_encrypt((unsigned char*)d, unenc_data, d_unenc_len, nonce, get_kEnc());
        }

        end = outside_util::curr_time();
        total_iee_add += trusted_util::time_elapsed_ms(start, end);

        start = outside_util::curr_time();

        // send batch to server
        outside_util::write(server_socket, &op, sizeof(unsigned char));
        outside_util::write(server_socket, &batch_size, sizeof(size_t));
        outside_util::write(server_socket, batch_buffer, batch_size * pair_size);

        end = outside_util::curr_time();
        total_server_add += trusted_util::time_elapsed_ms(start, end);

        to_recv -= batch_size;
    }

    start = outside_util::curr_time();

    free(kW);
    free(unenc_data);
    free(batch_buffer);
    free(nonce);

#ifdef VERBOSE
    //ocall_print_string("Finished update in IEE!\n");
#endif

    // output message
    *out_len = 1;
    *out = (unsigned char*)malloc(sizeof(unsigned char));
    (*out)[0] = 0x90;

    end = outside_util::curr_time();
    total_iee_add += trusted_util::time_elapsed_ms(start, end);
    count_add++;
}

void get_docs_from_server(vec_token *query, const unsigned count_words, const unsigned total_labels) {
#ifdef VERBOSE
    ocall_print_string("Requesting docs from server!\n");
#endif

    typedef struct {
        iee_token *tkn;
        unsigned counter_val;
    } label_request;

    untrusted_time start, end;
    start = outside_util::curr_time();

    // generate 0-filled nonce
    unsigned char* nonce = (unsigned char*)malloc(sizeof(unsigned char) * C_NONCESIZE);
    for(unsigned j = 0; j < C_NONCESIZE; j++)
        nonce[j] = 0x00;

    // used to hold all labels in random order
    label_request* labels = (label_request*)malloc(sizeof(label_request) * total_labels);
    memset(labels, 0, sizeof(label_request) * total_labels);

    // iterate over all the needed words, and then over all its occurences (given by the counter), and fills the requests array
    int k = 0;
    for(unsigned i = 0; i < vt_size(query); i++) {
        // ignore non-word tokens
        if(query->array[i].type != WORD_TOKEN)
            continue;

        // fisher-yates shuffle
        for(unsigned j = 0; j < query->array[i].counter; j++) {
            int r = c_random_uint_range(0, k+1);
            if(r != k) {
                labels[k].tkn = labels[r].tkn;
                labels[k].counter_val = labels[r].counter_val;
            }

            labels[r].tkn = &(*query).array[i];
            labels[r].counter_val = j;

            k++;
        }
    }

#ifdef VERBOSE
    ocall_print_string("Randomised positions!\n");
#endif

    /************************ ALLOCATE DATA STRUCTURES ************************/
    // buffer for server requests
    // (always max_batch_size, may not be filled if not needed)
    // op + batch_size + labels of H_BYTES len
    const size_t req_len = H_BYTES;
    unsigned char* req_buff = (unsigned char*)malloc(sizeof(unsigned char) * (sizeof(char) + sizeof(unsigned) + req_len * MAX_BATCH_SEARCH));

    // put the op code in the buffer
    const char op = '3';
    memcpy(req_buff, &op, sizeof(unsigned char));

    // buffer for encrypted server responses
    // contains the hmac for verif, the doc id, and the encryption's exp
    const size_t res_len = H_BYTES + 2 * sizeof(int) + C_EXPBYTES; // 44 + H_BYTES (32)
    unsigned char* res_buff = (unsigned char*)malloc(sizeof(unsigned char) * (res_len * MAX_BATCH_SEARCH));

    // buffer for decrypted server responses (holds one)
    const size_t dec_len = H_BYTES + 2 * sizeof(int);
    unsigned char* dec_buff = (unsigned char*)malloc(sizeof(unsigned char) * dec_len);
    /********************** END ALLOCATE DATA STRUCTURES **********************/

    unsigned label_pos = 0;
    unsigned batch_size = std::min(total_labels - label_pos, (unsigned)MAX_BATCH_SEARCH);

    end = outside_util::curr_time();
    total_iee_search += trusted_util::time_elapsed_ms(start, end);

    // request labels to server
    while (label_pos < total_labels) {
        start = outside_util::curr_time();

        // put batch_size in buffer
        memcpy(req_buff + sizeof(char), &batch_size, sizeof(unsigned));

        // aux pointer
        unsigned char* tmp = req_buff + sizeof(char) + sizeof(unsigned);

        // fill the buffer with labels
        for(unsigned i = 0; i < batch_size; i++) {
            label_request* req = &(labels[label_pos + i]);
            rbisen_c_hmac(tmp + i * req_len, (unsigned char*)&(req->counter_val), sizeof(int), req->tkn->kW);
        }

        /*printf("%d batch\n", batch_size);
        for(unsigned x = 0; x < batch_size*req_len; x++)
            printf("%02x", tmp[x]);
        printf(" : \n");*/

        end = outside_util::curr_time();
        total_iee_search += trusted_util::time_elapsed_ms(start, end);

        start = outside_util::curr_time();
        // send message to server and receive response
        outside_util::write(server_socket, req_buff, sizeof(char) + sizeof(unsigned) + req_len * batch_size);
        outside_util::read(server_socket, res_buff, res_len * batch_size);

        end = outside_util::curr_time();
        total_server_search += trusted_util::time_elapsed_ms(start, end);

        start = outside_util::curr_time();

        // decrypt and fill the destination data structs
        for(unsigned i = 0; i < batch_size; i++) {
            label_request* req = &labels[label_pos + i];
            rbisen_c_decrypt(dec_buff, res_buff + (res_len * i), res_len, nonce, get_kEnc());

            // aux pointer for req
            unsigned char* tmp_req = req_buff + sizeof(char) + sizeof(unsigned);

            // verify
            if(memcmp(dec_buff, tmp_req + i * req_len, H_BYTES)) {
                outside_util::printf("Label verification doesn't match! Exit\n");
                outside_util::exit(-1);
            }

            memcpy(&req->tkn->doc_frequencies.array[req->counter_val], dec_buff + H_BYTES, sizeof(int));
            memcpy(&req->tkn->docs.array[req->counter_val], dec_buff + H_BYTES + sizeof(int), sizeof(int));
        }

        label_pos += batch_size;
        batch_size = std::min(total_labels - label_pos, (unsigned)MAX_BATCH_SEARCH);

        end = outside_util::curr_time();
        total_iee_search += trusted_util::time_elapsed_ms(start, end);
    }

    start = outside_util::curr_time();
    free(labels);
    free(nonce);

    memset(req_buff, 0, req_len * MAX_BATCH_SEARCH);
    memset(res_buff, 0, res_len * MAX_BATCH_SEARCH);
    memset(dec_buff, 0, dec_len);

    free(req_buff);
    free(res_buff);
    free(dec_buff);

    end = outside_util::curr_time();
    total_iee_search += trusted_util::time_elapsed_ms(start, end);

#ifdef VERBOSE
    ocall_print_string("Got all docs from server!\n\n");
#endif
}

void calc_idf(vec_token *query, const unsigned total_docs) {
    for(unsigned i = 0; i < vt_size(query); i++) {
        iee_token* t = &query->array[i];

        if(t->type != 'w')
            continue;

        unsigned nr_docs = vi_size(&t->docs);
        t->idf = !nr_docs ? 0 : log10((double)total_docs / nr_docs);// nr_docs should have +1 to avoid 0 division

        //outside_util::printf("idf %c %f %lu %lu\n", t->type, t->idf, nr_docs, total_docs);
    }
}

void calc_tfidf(vec_token *query, vec_int* response_docs, void* results) {
    /*for (int i = 0; i < vi_size(response_docs); ++i)
        outside_util::printf("%d ", response_docs->array[i]);
    outside_util::printf("\n");*/

    /*outside_util::printf("query ");
    for (int i = 0; i < vt_size(query); ++i) {
        for (int j = 0; j < query->array[i].counter; ++j) {
            outside_util::printf("%d %d; ", query->array[i].docs.array[j], query->array[i].doc_frequencies.array[j]);
        }
        outside_util::printf("\n");
    }*/

    void* tmp = results;
    double max = 0;

    //iterate over the response docs
    for(unsigned i = 0; i < vi_size(response_docs); i++) {
        //outside_util::printf("doc id %d; ", response_docs->array[i]);
        int curr_doc = response_docs->array[i];
        double tf_idf = 0;

        // for each response doc, iterate over all terms and find who uses them
        for(unsigned j = 0; j < vt_size(query); j++) {
            iee_token* t = &query->array[j];

            if(t->type != 'w')
                continue;

            /*outside_util::printf("term ", j);
            for(unsigned k = 0; k < H_BYTES; k++)
                outside_util::printf("%02x", t->kW[k]);
            outside_util::printf("\n");*/

            int pos = vi_index_of(t->docs, curr_doc);
            //outside_util::printf("pos %d; ", pos);
            if(pos != -1) {
                tf_idf += t->doc_frequencies.array[pos] * t->idf;
                //outside_util::printf("freq %d ", t->doc_frequencies.array[pos]);
                max += tf_idf;
            }
            //outside_util::printf("\n");
        }

        // store tf-idf in result buffer
        memcpy(tmp, &curr_doc, sizeof(int));
        memcpy(tmp + sizeof(int), &tf_idf, sizeof(double));
        tmp = (char*)tmp + sizeof(int) + sizeof(double);
    }

    /*ocall_print_string("best");
    ocall_print_double(max);
    ocall_print_string("\n");*/

    /*hashmap_iterator_init(docs);
    while(hashmap_iterator_next(docs, &key, &val) != IT_END) {
        //ocall_print_unsigned(key);
        //ocall_print_string(" ");
        double x;//TODO free map and vals and etc
        int key;
        memcpy(&x, val, sizeof(double));
        memcpy(&x, val, sizeof(double));
        //ocall_print_double(x);
        //ocall_print_string("\n");
    }*/

}

int compare_results_rbisen(const void *a, const void *b) {
    double d_a = *((const double*) (a + sizeof(int)));
    double d_b = *((const double*) (b + sizeof(int)));

    if (d_a == d_b)
        return 0;
    else
        return d_a < d_b ? 1 : -1;
}

void search(bytes* out, size* out_len, uint8_t* in, const size in_len) {
    if(!count_search) {
        // prints only when no more updates
        outside_util::printf("-- BISEN add iee: %lfms %lu imgs--\n", total_iee_add, count_add);
        outside_util::printf("-- BISEN add uee w/ net: %lfms --\n", total_server_add);
    }

    outside_util::printf("## BISEN Search %lu ##\n", count_search);

    untrusted_time start, end;
    start = outside_util::curr_time();

    vec_token query;
    vt_init(&query, MAX_QUERY_TOKENS);

    unsigned nDocs = 0;
    unsigned count_words = 0; // useful for get_docs_from_server
    unsigned count_labels = 0; // useful for get_docs_from_server

    //read in
    int pos = 1;
    while(pos < in_len) {
        iee_token tkn;
        tkn.kW = NULL;

        iee_readFromArr(&tkn.type, 1, in, &pos);

        if(tkn.type == WORD_TOKEN) {
            count_words++;

            // read counter
            tkn.counter = (unsigned)iee_readIntFromArr(in, &pos);
            count_labels += tkn.counter;

            // read kW
            tkn.kW = (unsigned char*)malloc(sizeof(unsigned char) * H_BYTES);
            iee_readFromArr(tkn.kW, H_BYTES, in, &pos);

            // create the vector that will hold the docs
            vi_init(&tkn.docs, tkn.counter);
            vi_init(&tkn.doc_frequencies, tkn.counter);
            tkn.docs.counter = tkn.counter;
        } else if(tkn.type == META_TOKEN) {
            nDocs = (unsigned)iee_readIntFromArr(in, &pos);
            continue;
        }

        vt_push_back(&query, tkn);
    }

    end = outside_util::curr_time();
    total_iee_search += trusted_util::time_elapsed_ms(start, end);

    // get documents from uee
    get_docs_from_server(&query, count_words, count_labels);

#ifdef VERBOSE
    /*ocall_strprint("parsed: ");
    for(unsigned i = 0; i < vt_size(query); i++) {
        iee_token x = query.array[i];
        if(x.type == WORD_TOKEN) {
            ocall_printf("%s (", x.word);
            for(unsigned i = 0; i < vi_size(x.docs); i++) {
                if(i < vi_size(x.docs) - 1)
                    ocall_printf("%i,", x.docs.array[i]);
                else
                    ocall_printf("%i); ", x.docs.array[i]);
            }
        } else {
            ocall_printf("%c ", x.type);
        }
    }
    ocall_print_string("\n\n");*/
#endif

    start = outside_util::curr_time();

    // ensure aux_bool has space for all docs
    if(nDocs > last_ndocs) {
        //ocall_print_string("Realloc aux bool buffer\n");
        aux_bool = (unsigned char *)realloc(aux_bool, sizeof(unsigned char) * nDocs);
        last_ndocs = nDocs;
    }

    /*outside_util::printf("query ");
    for (int i = 0; i < vt_size(&query); ++i) {
        for (int j = 0; j < query.array[i].counter; ++j) {
            outside_util::printf("%d %d; ", query.array[i].docs.array[j], query.array[i].doc_frequencies.array[j]);
        }
        outside_util::printf("\n");
    }*/

    //calculate boolean formula
    vec_int response_docs = evaluate(query, nDocs, aux_bool);
    const size_t original_res_size = vi_size(&response_docs);

    /*for (int i = 0; i < vi_size(&response_docs); ++i)
        outside_util::printf("%d ", response_docs.array[i]);
    outside_util::printf("\n");*/

    void* results_buffer = NULL;
    if(vi_size(&response_docs)) {
        // calculate idf for each term
        calc_idf(&query, nDocs);

        results_buffer = malloc(vi_size(&response_docs) * (sizeof(int) + sizeof(double)));

        // calculate tf-idf for each document
        calc_tfidf(&query, &response_docs, results_buffer);

        /*void* tmp1 = results_buffer;
        for (int j = 0; j < vi_size(response_docs); ++j) {
            int a; memcpy(&a, tmp1, sizeof(int));
            double b; memcpy(&b, tmp1 + sizeof(int), sizeof(double));

            ocall_print_unsigned(a);
            ocall_print_string(" ");
            ocall_print_double(b);
            ocall_print_string("\n");
            tmp1 = (char*)tmp1 + sizeof(int) + sizeof(double);
        }
        ocall_print_string("--------------\n");*/
        qsort(results_buffer, vi_size(&response_docs), sizeof(int) + sizeof(double), compare_results_rbisen);
        /*void* tmp = results_buffer;
        for (int j = 0; j < 15; ++j) {
            int a; memcpy(&a, tmp, sizeof(int));
            double b; memcpy(&b, tmp + sizeof(int), sizeof(double));

            ocall_print_unsigned(a);
            ocall_print_string(" ");
            ocall_print_double(b);
            ocall_print_string("\n");
            tmp = (char*)tmp + sizeof(int) + sizeof(double);
        }*/
    }

    const unsigned threshold = 10;//vi_size(response_docs);

    // return query results
    unsigned long long output_size = sizeof(unsigned) + threshold * (sizeof(int) + sizeof(double));
    *out_len = sizeof(unsigned char) * output_size;
    *out = (unsigned char*)malloc(*out_len);
    memset(*out, 0, output_size);

    // add nr of elements at beggining, useful if they are less than threshold
    const unsigned elements = std::min(threshold, vi_size(&response_docs));
    memcpy(*out, &elements, sizeof(unsigned));

    void* read_tmp = results_buffer;
    void* write_tmp = *out + sizeof(unsigned);

    for(unsigned i = 0; i < elements; i++) {
        //outside_util::printf("%d ", response_docs.array[i]);
        memcpy(write_tmp, read_tmp, sizeof(int));
        memcpy(write_tmp + sizeof(int), read_tmp + sizeof(int), sizeof(double));

        read_tmp = (char*)read_tmp + sizeof(int) + sizeof(double);
        write_tmp = (char*)write_tmp + sizeof(int) + sizeof(double);
    }
    //outside_util::printf("\n-----------------\n");

    // free the buffers in iee_tokens
    for(unsigned i = 0; i < vt_size(&query); i++) {
        iee_token t = query.array[i];

        // vi_destroy(&t.doc_frequencies);
        // vi_destroy(&t.docs);
        //free(t.doc_frequencies.array);
        //free(t.docs.array);
        free(t.kW);
    }

    vt_destroy(&query);
    vi_destroy(&response_docs);

    if(results_buffer)
        free(results_buffer);

    end = outside_util::curr_time();
    total_iee_search += trusted_util::time_elapsed_ms(start, end);

    outside_util::printf("-- BISEN search iee: %lfms (%lu docs)--\n", total_iee_search, original_res_size);
    outside_util::printf("-- BISEN search uee w/ net: %lfms --\n", total_server_search);

    total_iee_search = 0;
    total_server_search = 0;
    count_search++;

    //outside_util::printf("Finished Search!\n-----------------\n");

    benchmarking_print();
}
