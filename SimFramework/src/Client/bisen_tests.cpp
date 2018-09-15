#include "bisen_tests.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

extern "C" {
#include "rbisen/Utils.h"
#include "rbisen/types.h"
}

void bisen_setup(secure_connection* conn, SseClient* client) {
    unsigned char* data_bisen;
    unsigned long long data_size_bisen;

    // setup
    data_size_bisen = client->generate_setup_msg(&data_bisen);
    iee_comm(conn, data_bisen, data_size_bisen);
    free(data_bisen);
}

void bisen_update(secure_connection* conn, SseClient* client, char* bisen_doc_type, unsigned nr_docs, std::vector<std::string> doc_paths) {
    unsigned char* data_bisen;
    unsigned long long data_size_bisen;

    printf("Update type: %s\n", bisen_doc_type);

    size_t nr_updates = 0;
    for (const string doc : doc_paths) {
        vector<map<string, int>> docs;

        if(!strcmp(bisen_doc_type, "wiki")) {
            // extract keywords from a 1M, multiple article, file
            docs = client->extract_keywords_frequency_wiki(doc);
        } else if(!strcmp(bisen_doc_type, "normal")) {
            // one file is one document
            docs.push_back(client->extract_keywords_frequency(doc));
        } else {
            printf("Document type not recognised\n");
            exit(0);
        }

        nr_updates += docs.size();

        for (const map<string, int> text : docs) {
            // generate the byte* to send to the server
            data_size_bisen = client->add_new_document(text, &data_bisen);
            iee_comm(conn, data_bisen, data_size_bisen);
            free(data_bisen);
        }

        if (nr_updates >= nr_docs) {
            printf("Breaking, done enough updates (%lu/%u)\n", nr_updates, nr_docs);
            break;
        }
    }
}

void bisen_search(secure_connection* conn, SseClient* client, vector<string> queries) {
    unsigned char* data_bisen;
    size_t data_size_bisen;

    for (unsigned k = 0; k < queries.size(); k++) {
        string query = queries[k];
        printf("\n----------------------------\n");
        printf("Query %d: %s\n", k, query.c_str());

        data_size_bisen = client->search(query, &data_bisen);
        iee_send(conn, data_bisen, data_size_bisen);
        free(data_bisen);

        uint8_t* bisen_out;
        size_t bisen_out_len;
        iee_recv(conn, &bisen_out, &bisen_out_len);

        unsigned n_docs;
        memcpy(&n_docs, bisen_out, sizeof(int));
        printf("Number of docs: %d\n", n_docs);

        for (unsigned i = 0; i < n_docs; ++i) {
            int d;
            double s;
            memcpy(&d, bisen_out + sizeof(unsigned) + i * (sizeof(int) + sizeof(double)), sizeof(int));
            memcpy(&s, bisen_out + sizeof(unsigned) + i * (sizeof(int) + sizeof(double)) + sizeof(int), sizeof(double));
            printf("%d %f\n", d, s);
        }

        free(bisen_out);
    }
}