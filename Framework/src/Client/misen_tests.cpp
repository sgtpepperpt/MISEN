#include "misen_tests.h"

#include "definitions.h"

#include "util.h"

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;

string query_from_file(string path) {
    string query;

    ifstream file(path);
    string line;
    if (file.is_open()) {
        while (getline(file,line)) {
            // replace spaces
            replace(line.begin(), line.end(), ' ', '_');

            // erase \r from mirflickr
            line = line.substr(0, line.size() - 1);

            if(query.length() == 0)
                query = line;
            else
                query.append(" && " + line);
        }

        file.close();
    } else {
        printf("Unable to open file %s\n", path.c_str());
        exit(1);
    }

    return query;
}

void misen_search(secure_connection* conn, SseClient* client, Ptr<SIFT> extractor, vector<pair<string, string>> queries) {
    unsigned count = 0;
    for (pair<string, string> query : queries) {
        string text_file_path = query.first;
        string img_file_path = query.second;

        //printf("\n----------------------------\n");
        //printf("Query %d: %s %s\n", count++, text_file_path.c_str(), img_file_path.c_str());

        size_t query_bisen_len, query_visen_len;
        uint8_t* query_bisen, *query_visen;

        string bool_query = query_from_file(text_file_path);
        if(bool_query.size() == 0)
            bool_query = "empty";

        query_bisen_len = client->search(bool_query, &query_bisen);
        search(&query_visen, &query_visen_len, extractor, img_file_path);

        uint8_t multimodal_query[sizeof(unsigned char) + 2 * sizeof(size_t) + query_bisen_len + query_visen_len];
        multimodal_query[0] = OP_MISEN_QUERY;
        memcpy(multimodal_query + sizeof(unsigned char), &query_bisen_len, sizeof(size_t));
        memcpy(multimodal_query + sizeof(unsigned char) + sizeof(size_t), query_bisen, query_bisen_len);

        memcpy(multimodal_query + sizeof(unsigned char) + sizeof(size_t) + query_bisen_len, &query_visen_len, sizeof(size_t));
        memcpy(multimodal_query + sizeof(unsigned char) + 2 * sizeof(size_t) + query_bisen_len, query_visen, query_visen_len);

        free(query_bisen);
        free(query_visen);

        iee_send(conn, multimodal_query, sizeof(unsigned char) + 2 * sizeof(size_t) + query_bisen_len + query_visen_len);

        uint8_t* res;
        size_t res_len;
        iee_recv(conn, &res, &res_len);

        unsigned nr_docs;
        memcpy(&nr_docs, res, sizeof(unsigned));
        //printf("Number of docs: %u\n", nr_docs);

        for (unsigned i = 0; i < nr_docs; ++i) {
            unsigned d;
            double s;
            memcpy(&d, res + sizeof(unsigned) + i * (sizeof(unsigned) + sizeof(double)), sizeof(unsigned));
            memcpy(&s, res + sizeof(unsigned) + i * (sizeof(unsigned) + sizeof(double)) + sizeof(unsigned), sizeof(double));
            //printf("%u %f\n", d, s);
        }

        free(res);
    }
}

vector<pair<string, string>> generate_multimodal_queries(vector<string> txt_paths, vector<string> img_paths, int nr_queries) {
    vector<pair<string, string>> multimodal_queries;

    for (int i = 0; i < nr_queries; ++i)
        multimodal_queries.push_back(make_pair(txt_paths[i], img_paths[i]));

    return multimodal_queries;
}
