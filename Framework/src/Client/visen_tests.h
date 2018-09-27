#ifndef __CLIENT_TRAINING_H
#define __CLIENT_TRAINING_H

#include "untrusted_util.h"
#include "rbisen/SseClient.hpp"

#include "ImageSearch.h"

void visen_setup(secure_connection* conn, size_t desc_len, unsigned visen_nr_clusters, const char* train_technique);
void visen_train_client_kmeans(secure_connection* conn, size_t desc_len, unsigned visen_nr_clusters, char* visen_train_mode, char* visen_centroids_file, const char* visen_dataset_dir, descriptor_t descriptor);
void visen_train_iee_kmeans(secure_connection* conn, char* visen_train_mode, descriptor_t descriptor, const vector<string> files);
void visen_train_client_lsh(secure_connection* conn, char* visen_train_mode, size_t desc_len, unsigned visen_nr_clusters);
void visen_add_files(secure_connection* conn, descriptor_t descriptor, const vector<string> files);

#endif
