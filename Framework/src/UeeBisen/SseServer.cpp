//
//  MainUEE.cpp
//  BooleanSSE
//
//  Created by Bernardo Ferreira on 16/11/16.
//  Copyright © 2016 Bernardo Ferreira. All rights reserved.
//

#include "SseServer.hpp"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include "untrusted_util.h"
#include "definitions.h"

#if CASSANDRA
#include "cassie.h"

CassSession* session;
CassCluster* cluster;
#endif

using namespace std;

typedef struct client_data {
    int socket;
} client_data;
int c = 0;
void* process_client(void* args) {
    client_data* data = (client_data*)args;
    int client_socket = data->socket;
    free(data);

    uee_map I;

    struct timeval start, end;
    double total_add_time = 0, total_search_time = 0;
    double total_add_time_network = 0, total_search_time_network = 0;
    size_t nr_search_batches = 0;
    size_t count_searches = 0, count_adds = 0;

    while (1) {
        unsigned char op;
        untrusted_util::socket_receive(client_socket, &op, sizeof(unsigned char));

        switch (op) {
            //generate_setup_msg
            case OP_UEE_INIT: {
                // delete existing map, only useful for reruns while debugging

                /*if(!I.empty()) {
                    //#ifdef VERBOSE
                    printf("Size before: %lu\n", I.size());
                    //#endif

                    uee_map::iterator it;
                    for(it = I.begin(); it != I.end(); ++it) {
                        free(it->first);
                        free(it->second);
                    }

                    I.clear();
                }*/

                #ifdef VERBOSE
                printf("Finished Setup!\n");
                #endif
                break;
            }
            // add
            case OP_UEE_ADD: {

                #ifdef VERBOSE
                printf("Started Add!\n");
                #endif

                /*if(count_adds++ % 1000 == 0)
                    printf("add %lu\n", count_adds);*/

                gettimeofday(&start, NULL);

                size_t nr_labels;
                untrusted_util::socket_receive(client_socket, &nr_labels, sizeof(size_t));

                uint8_t* buffer = (uint8_t*)malloc(nr_labels * (l_size + d_size));
                untrusted_util::socket_receive(client_socket, buffer, nr_labels * (l_size + d_size));

                gettimeofday(&end, NULL);
                total_add_time_network += untrusted_util::time_elapsed_ms(start, end);

                gettimeofday(&start, NULL);

#if CASSANDRA
                const CassPrepared* prepared = NULL;
#endif
                for(size_t i = 0; i < nr_labels; i++) {
                    void* l = buffer + i * (l_size + d_size);
                    void* d = buffer + i * (l_size + d_size) + l_size;

                    /*for(unsigned x = 0; x < d_size; x++)
                        printf("%02x", ((uint8_t*)d)[x]);
                    printf(" : d\n");*/
#if CASSANDRA
                    if (prepare_insert_into_batch(session, &prepared) == CASS_OK)
                        insert_into_batch_with_prepared(session, prepared, nr_labels, buffer);
                    cass_prepared_free(prepared);
#else
                    I[l] = d;
#endif
                }

                gettimeofday(&end, NULL);
                total_add_time += untrusted_util::time_elapsed_ms(start, end);

                #ifdef VERBOSE
                printf("Finished Add!\n");
                #endif
                break;
            }
            // search - get index positions
            case OP_UEE_SEARCH: {
                #ifdef VERBOSE
                printf("Started Search!\n");
                #endif

                gettimeofday(&start, NULL);

                unsigned nr_labels;
                untrusted_util::socket_receive(client_socket, &nr_labels, sizeof(unsigned));

                //cout << "batch_size " << batch_size << endl;

                unsigned char* label = new unsigned char[l_size * nr_labels];
                untrusted_util::socket_receive(client_socket, label, l_size * nr_labels);

                gettimeofday(&end, NULL);
                total_search_time_network += untrusted_util::time_elapsed_ms(start, end);

                gettimeofday(&start, NULL);

                uint8_t* buffer = (unsigned char*)malloc(d_size * nr_labels);

#if CASSANDRA
                uint8_t* res = (uint8_t*)malloc(d_size);
                const CassPrepared* prepared = NULL;
#endif
                // send the labels for each word occurence
                for (unsigned i = 0; i < nr_labels; i++) {
                    if(!I[label + i * l_size]) {
                        printf("Label not found! Exit\n");
                        exit(1);
                    }

#if CASSANDRA
                    if (prepare_select_from_batch(session, &prepared) == CASS_OK)
                        select_from_batch_with_prepared(session, prepared, label + i * l_size, &res);
                    cass_prepared_free(prepared);

                    if(memcmp(res, I[label + i * l_size], d_size)) {
                        printf("not the same!\n");
                        exit(1);
                    }

                    memcpy(buffer + i * d_size, res, d_size);
#else
                    memcpy(buffer + i * d_size, I[label + i * l_size], d_size);
#endif
                }

                gettimeofday(&end, NULL);
                total_search_time += untrusted_util::time_elapsed_ms(start, end);

                gettimeofday(&start, NULL);

                untrusted_util::socket_send(client_socket, buffer, d_size * nr_labels);
                free(buffer);

                gettimeofday(&end, NULL);
                total_search_time_network += untrusted_util::time_elapsed_ms(start, end);

                nr_search_batches++;

                #ifdef VERBOSE
                printf("Finished Search!\n");
                #endif
                break;
            }
            case '4': {
                // this instruction is for benchmarking only and can be safely
                // removed if wanted
                if(!count_searches) {
                    printf("BISEN SERVER: total add = %6.3lf ms\n", total_add_time);
                    printf("BISEN SERVER: total add network = %6.3lf ms\n", total_add_time_network);
                    printf("BISEN SERVER: size index: %lu\n", I.size());
                }

                printf("## STATS %lu ##\n", count_searches++);
                printf("BISEN SERVER: seen search batches: %lu\n", nr_search_batches);
                printf("BISEN SERVER: TOTAL search = %6.3lf ms\n", total_search_time + total_search_time_network);
                printf("BISEN SERVER: search process = %6.3lf ms\n", total_search_time);
                printf("BISEN SERVER: search network = %6.3lf ms\n", total_search_time_network);
                total_search_time = 0;
                total_search_time_network = 0;
                break;
            }
            default:
                printf("SseServer unkonwn command: %02x\n", op);
        }
    }
}

SseServer::SseServer() {
    const int server_port = 7899;
#if CASSANDRA
    char* hosts = "127.0.0.1";

    session = cass_session_new();
    cluster = create_cluster(hosts);

    if (connect_session(session, cluster) != CASS_OK) {
        cass_cluster_free(cluster);
        cass_session_free(session);
        exit(1);
    }

    execute_query(session, "CREATE KEYSPACE IF NOT EXISTS isen WITH REPLICATION = {'class' : 'SimpleStrategy', 'replication_factor' : 1};");
    execute_query(session, "DROP TABLE IF EXISTS isen.pairs;");
    execute_query(session, "CREATE TABLE isen.pairs (key blob, value blob, PRIMARY KEY (key));");
#endif
    struct sockaddr_in server_addr;
    memset(&server_addr, 0x00, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int listen_socket;
    if ((listen_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Could not create socket!\n");
        exit(1);
    }

    int res = 1;
    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &res, sizeof(res)) == -1) {
        printf("Could not set socket options!\n");
        exit(1);
    }

    if ((bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0) {
        printf("Could not bind socket!\n");
        exit(1);
    }

    if (listen(listen_socket, 16) < 0) {
        printf("Could not open socket for listening!\n");
        exit(1);
    }

    //start listening for iee calls
    printf("Finished Server init! Gonna start listening for IEE requests!\n");

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = 0;

    printf("Listening for requests...\n");

    while (true) {
        client_data* data = (client_data*)malloc(sizeof(client_data));

        if ((data->socket = accept(listen_socket, (struct sockaddr*)&client_addr, &client_addr_len)) < 0) {
            printf("Accept failed!\n");
            exit(1);
        }

        int flag = 1;
        setsockopt(data->socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

        int iMode = 0;
        ioctl(data->socket, FIONBIO, &iMode);

        printf("------------------------------------------\nClient connected (%s)\n", inet_ntoa(client_addr.sin_addr));

        pthread_t tid;
        pthread_create(&tid, NULL, process_client, data);
    }
#if CASSANDRA
    CassFuture* close_future = cass_session_close(session);
    cass_future_wait(close_future);
    cass_future_free(close_future);

    cass_cluster_free(cluster);
    cass_session_free(session);
#endif
}

SseServer::~SseServer() {
    //delete[] I;
    //close(clientSock);
}

