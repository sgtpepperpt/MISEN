#include "Server.h"

#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "definitions.h"
#include "untrusted_util.h"

using namespace std;

/****************************************************** NET DATA ******************************************************/
static int server_socket;

typedef struct client_data {
    int socket;
} client_data;

static void close_all(int signum) {
    close(server_socket);
    fflush(NULL);
    exit(0);
}
/****************************************************** NET DATA ******************************************************/

void ok_response(uint8_t** out, size_t* out_len) {
    *out_len = 1;
    *out = (uint8_t*)malloc(sizeof(unsigned char));
    *out[0] = 'x';
}

// map label and value sizes
const size_t l_size = 32;
const size_t d_size = 48;

tbb::concurrent_unordered_map<void*, void*, VoidHash<l_size>, VoidEqual<l_size>> I;

static void repository_clear() {
    for (tbb::concurrent_unordered_map<void*, void*, VoidHash<l_size>, VoidEqual<l_size>>::iterator it = I.begin(); it != I.end() ; ++it) {
        free(it->first);
        free(it->second);
    }

    I.clear();
}

void* process_client(void* args) {
    const int socket = ((client_data *) args)->socket;
    free(args);

    long total_add_time = 0;

    while (1) {
        size_t in_len;
        untrusted_util::socket_receive(socket, &in_len, sizeof(size_t));

        uint8_t* in_buffer = (uint8_t*)malloc(in_len);
        untrusted_util::socket_receive(socket, in_buffer, in_len);

        uint8_t* out = NULL;
        size_t out_len = 0;

        const unsigned char op = in_buffer[0];
        switch (op) {
            case OP_UEE_INIT: {
                // clean previous elements, if any
                repository_clear();

                printf("Server init\n");
                //memcpy(&l_size, in_buffer + sizeof(uint8_t), sizeof(size_t));
                //memcpy(&d_size, in_buffer + sizeof(uint8_t) + sizeof(size_t), sizeof(size_t));
                //TODO clean previous map data
                ok_response(&out, &out_len);
                break;
            }
            case OP_UEE_ADD: {
                printf("Add image (c. %lu)\n", I.size());

                struct timeval start, end;
                gettimeofday(&start, NULL);

                uint8_t* tmp = in_buffer + sizeof(unsigned char);

                size_t nr_labels;
                memcpy(&nr_labels, tmp, sizeof(size_t));
                tmp += sizeof(size_t);

                for (size_t i = 0; i < nr_labels; ++i) {
                    uint8_t* label = (uint8_t*)malloc(l_size);
                    memcpy(label, tmp, l_size);
                    tmp += l_size;

                    uint8_t* d = (uint8_t*)malloc(d_size);
                    memcpy(d, tmp, d_size);
                    tmp += d_size;

                    I[label] = d;
                }

                ok_response(&out, &out_len);
                gettimeofday(&end, NULL);
                total_add_time += untrusted_util::time_elapsed_ms(start, end);

                break;
            }
            case OP_UEE_SEARCH: {
                printf("Search\n");

                uint8_t* tmp = in_buffer + sizeof(unsigned char);

                size_t nr_labels;
                memcpy(&nr_labels, tmp, sizeof(size_t));
                tmp += sizeof(size_t);

                out_len = nr_labels * d_size;
                out = (uint8_t*)malloc(out_len);

                for (size_t i = 0; i < nr_labels; ++i) {
                    uint8_t label[l_size];
                    memcpy(label, tmp, l_size);
                    tmp += l_size;

                    if(!I[label]) {
                        untrusted_util::debug_printbuf(label, l_size);
                        printf("Map error: %lu/%lu", i, nr_labels);
                        exit(1);
                    }

                    memcpy(out + i * d_size, I[label], d_size);
                }

                break;
            }
            case OP_UEE_READ_MAP: {
                printf("Reading map from disk\n");
                FILE* map_file = fopen("map_data", "r");
                if(!map_file){
                    printf("Error opening map file\n!");
                    exit(1);
                }

                size_t nr_pairs;
                fread(&nr_pairs, sizeof(size_t), 1, map_file);
                printf("nr_pairs %lu\n", nr_pairs);

                int count = 0, ccount = 0;
                for(size_t i = 0; i < nr_pairs; ++i) {
                    void* label = (uint8_t*)malloc(l_size);

                    uint8_t tmp_a[l_size]; // TODO this needs to vanish, fread directly into malloc'd buffers was not working
                    fread(tmp_a, l_size, 1, map_file);
                    memcpy(label, tmp_a, l_size);

                    untrusted_util::debug_printbuf((uint8_t*)label, l_size);

                    void* d = (uint8_t*)malloc(d_size);
                    uint8_t tmp_b[d_size];
                    fread(tmp_b, d_size, 1, map_file);
                    memcpy(d, tmp_b, d_size);

                    if(((uint8_t*)label)[0] == 0x56 && ((uint8_t*)label)[1] == 0x97 && ((uint8_t*)label)[5] == 0xfa)
                        count++;
                    else ccount++;


                    I[label] = d;
                }
                printf("wcount %d ccount %d\n", count, ccount);
                fclose(map_file);
                printf("done!\n");
                break;
            }
            case OP_UEE_WRITE_MAP: {
                printf("Writing map to disk\n");
                FILE* map_file = fopen("map_data", "w");
                if(!map_file){
                    printf("Error opening map file\n!");
                    exit(1);
                }

                const size_t nr_pairs = I.size();
                fwrite(&nr_pairs, sizeof(size_t), 1, map_file);

                int count = 0, ccount = 0;

                for (tbb::concurrent_unordered_map<void*, void*, VoidHash<l_size>, VoidEqual<l_size>>::iterator it = I.begin(); it != I.end() ; ++it) {
                    if(!((uint8_t*)it->first)[0] && !((uint8_t*)it->first)[1] && !((uint8_t*)it->first)[5])
                        count++;
                    else ccount++;

                    fwrite(it->first, sizeof(l_size), 1, map_file);
                    fwrite(it->second, sizeof(d_size), 1, map_file);
                }

                printf("wcount %d ccount %d\n", count, ccount);
                fclose(map_file);
                printf("done!\n");
                break;
            }
            case OP_UEE_CLEAR: {
                printf("Clear\n");
                repository_clear();
                break;
            }
            default:
                printf("SseServer unkonwn command: %02x\n", op);
        }
        free(in_buffer);

        // send response
        untrusted_util::socket_send(socket, &out_len, sizeof(size_t));
        untrusted_util::socket_send(socket, out, out_len);

        free(out);
    }

    printf("Client closed\n");
    close(socket);
}

int main(int argc, const char ** argv) {
    const int server_port = UEE_PORT;

    // register signal handler
    signal(SIGINT, close_all);

    // initialise listener socket
    int server_socket = untrusted_util::init_server(server_port);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = 0;

    printf("Listening for requests...\n");
    while (1) {
        client_data *data = (client_data *)malloc(sizeof(client_data));

        if ((data->socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            printf("Accept failed!\n");
            break;
        }

        printf("Client connected (%s)\n", inet_ntoa(client_addr.sin_addr));

        pthread_t tid;
        pthread_create(&tid, NULL, process_client, data);
    }

    close(server_socket);
}
