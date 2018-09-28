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

#if STORAGE == STORAGE_CASSANDRA
#include "cassie.h"

CassSession* session;
CassCluster* cluster;
#elif STORAGE == STORAGE_REDIS
#include <hiredis/hiredis.h>
#endif

using namespace std;

/*****************************************************************************/
typedef struct search_res {
    double processing = 0;
    double network = 0;
    size_t batches = 0;
    size_t nr_labels = 0;
} search_res;

/*****************************************************************************/
typedef struct client_data {
    int socket;
} client_data;
/*****************************************************************************/

int c = 0;
void* process_client(void* args) {
    client_data* data = (client_data*)args;
    int client_socket = data->socket;
    free(data);

#if STORAGE == STORAGE_REDIS
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            printf("Error: %s\n", c->errstr);
            // handle error
        } else {
            printf("Can't allocate redis context\n");
        }
    }
#elif STORAGE == STORAGE_MAP
    uee_map I;
#endif

    vector<search_res> search_results;

    struct timeval start;
    double total_add_time = 0;
    double total_add_time_network = 0;
    size_t count_adds = 0;

    while (1) {
        unsigned char op;
        untrusted_util::socket_receive(client_socket, &op, sizeof(unsigned char));

        switch (op) {
            //generate_setup_msg
            case OP_UEE_INIT: {
                // delete existing map, only useful for reruns while debugging

#if STORAGE == STORAGE_CASSANDRA

#elif STORAGE == STORAGE_REDIS
                redisReply* reply = (redisReply*)redisCommand(c, "FLUSHALL");
                if (!reply) {
                    printf("error redis flushall!\n");
                }
#elif STORAGE == STORAGE_MAP
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
#endif

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

                // receive data from iee
                start = untrusted_util::curr_time();
                size_t nr_labels;
                untrusted_util::socket_receive(client_socket, &nr_labels, sizeof(size_t));

                uint8_t* buffer = (uint8_t*)malloc(nr_labels * pair_len);
                untrusted_util::socket_receive(client_socket, buffer, nr_labels * pair_len);
                total_add_time_network += untrusted_util::time_elapsed_ms(start, untrusted_util::curr_time());

                start = untrusted_util::curr_time();

#if STORAGE == STORAGE_CASSANDRA
                const CassPrepared* prepared = NULL;
                if (prepare_insert_into_batch(session, &prepared) == CASS_OK)
                    insert_into_batch_with_prepared(session, prepared, nr_labels, buffer);
                cass_prepared_free(prepared);
#elif STORAGE == STORAGE_REDIS
                for(size_t i = 0; i < nr_labels; i++) {
                    void* l = buffer + i * pair_len;
                    void* d = buffer + i * pair_len + l_size;
                    redisAppendCommand(c, "SET %b %b", l, (size_t)l_size, d, (size_t)d_size);
                }
#elif STORAGE == STORAGE_MAP
                for(size_t i = 0; i < nr_labels; i++) {
                    void* l = buffer + i * pair_len;
                    void* d = buffer + i * pair_len + l_size;
                    I[l] = d;
                }
#endif

#if STORAGE == STORAGE_REDIS
                for (size_t i = 0; i < nr_labels; i++) {
                    redisReply* reply;
                    redisGetReply(c,(void**)&reply);

                    freeReplyObject(reply);
                }
#endif
                total_add_time += untrusted_util::time_elapsed_ms(start, untrusted_util::curr_time());
                count_adds++;

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

                // receive information from iee
                start = untrusted_util::curr_time();
                unsigned nr_labels;
                untrusted_util::socket_receive(client_socket, &nr_labels, sizeof(unsigned));

                //cout << "batch_size " << nr_labels << endl;

                uint8_t* label = new uint8_t[l_size * nr_labels];
                untrusted_util::socket_receive(client_socket, label, l_size * nr_labels);
                search_results.back().network += untrusted_util::time_elapsed_ms(start, untrusted_util::curr_time());

                start = untrusted_util::curr_time();
                uint8_t* buffer = (unsigned char*)malloc(d_size * nr_labels);

#if STORAGE == STORAGE_CASSANDRA
                uint8_t* res = (uint8_t*)malloc(d_size);
                const CassPrepared* prepared = NULL;

                // send the labels for each word occurence
                for (unsigned i = 0; i < nr_labels; i++) {
                    if (prepare_select_from_batch(session, &prepared) == CASS_OK)
                        select_from_batch_with_prepared(session, prepared, label + i * l_size, &res);
                    cass_prepared_free(prepared);

                    /*if(memcmp(res, I[label + i * l_size], d_size)) {
                        printf("not the same!\n");
                        exit(1);
                    }*/

                    memcpy(buffer + i * d_size, res, d_size);

                }
#elif STORAGE == STORAGE_REDIS
                // send the labels for each word occurence
                for (unsigned i = 0; i < nr_labels; i++) {
                    redisReply* reply = (redisReply*)redisCommand(c, "GET %b", label + i * l_size, l_size);
                    if (!reply) {
                         printf("error redis get!\n");
                    }

                    memcpy(buffer + i * d_size, reply->str, d_size);
                    freeReplyObject(reply);
                }
#elif STORAGE == STORAGE_MAP
                // send the labels for each word occurence
                for (unsigned i = 0; i < nr_labels; i++) {
                    if(!(I[label + i * l_size])) {
                        printf("Label not found! Exit\n");
                        exit(1);
                    }

                    memcpy(buffer + i * d_size, I[label + i * l_size], d_size);
                }
#endif
                delete[] label;

                search_results.back().processing += untrusted_util::time_elapsed_ms(start, untrusted_util::curr_time());

                // answer back to iee
                start = untrusted_util::curr_time();
                untrusted_util::socket_send(client_socket, buffer, d_size * nr_labels);
                free(buffer);
                search_results.back().network += untrusted_util::time_elapsed_ms(start, untrusted_util::curr_time());

                search_results.back().batches++;
                search_results.back().nr_labels += nr_labels;
                break;
            }
            case '4': {
                // this instruction is for benchmarking only and can be safely
                // removed if wanted
                size_t db_size = 0;

#if STORAGE == STORAGE_CASSANDRA
                db_size = get_size(session);
#elif STORAGE == STORAGE_REDIS
                redisReply* reply = (redisReply*)redisCommand(c, "DBSIZE");
                if (!reply) {
                    printf("error redis getdbsize!\n");
                }

                memcpy(&db_size, (size_t*)&(reply->integer), sizeof(size_t));
#elif STORAGE == STORAGE_MAP
                db_size = I.size();
#endif
                printf("BISEN SERVER: total add = %6.3lf ms\n", total_add_time);
                printf("BISEN SERVER: total add network = %6.3lf ms\n", total_add_time_network);
                printf("BISEN SERVER: total add batches = %lu\n", count_adds);
                printf("BISEN SERVER: size index: %lu\n", db_size);

                printf("-- BISEN STORAGE SEARCHES --\n");
                printf("TOTAL\tPROCESSING\tNETWORK\tBATCHES\tLABELS\n");
                for (search_res r : search_results) {
                    printf("%lf\t%lf\t%lf\t%lu\t%lu\n", r.network + r.processing, r.processing, r.network, r.batches, r.nr_labels);
                }

                // reset
                search_results.clear();

                break;
            }
            case '5': {
                // this instruction is for benchmarking only and can be safely
                // removed if wanted

                // adds a new search benchmarking record
                search_res r;
                search_results.push_back(r);

                break;
            }
            default: {
                printf("SseServer unkonwn command: %02x\n", op);
                exit(1);
            }
        }
    }
}

SseServer::SseServer() {
    const int server_port = 7899;
#if STORAGE == STORAGE_CASSANDRA
    // only cassandra has a global session
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
#if STORAGE == STORAGE_CASSANDRA
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

