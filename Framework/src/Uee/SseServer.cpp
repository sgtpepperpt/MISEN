#include "SseServer.h"

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

const size_t l_size = 32;
const size_t d_size = 44;
unordered_map<void*, void*, VoidHash<l_size>, VoidEqual<l_size>> I;

void* process_client(void* args) {
    const int socket = ((client_data *) args)->socket;
    free(args);

    long total_add_time = 0;

    while (1) {
        size_t in_len;
        socket_receive(socket, &in_len, sizeof(size_t));

        uint8_t* in_buffer = (uint8_t*)malloc(in_len);
        socket_receive(socket, in_buffer, in_len);

        uint8_t* out = NULL;
        size_t out_len = 0;

        unsigned char op = in_buffer[0];

        switch (op) {
            case 'i': {
                printf("Server init\n");
                //memcpy(&l_size, in_buffer + sizeof(uint8_t), sizeof(size_t));
                //memcpy(&d_size, in_buffer + sizeof(uint8_t) + sizeof(size_t), sizeof(size_t));
                //TODO clean previous map data
                ok_response(&out, &out_len);
                break;
            }
            case 'a': {
                printf("Add image\n");

                struct timeval start, end;
                gettimeofday(&start, NULL);

                uint8_t* label = (uint8_t*)malloc(l_size);
                uint8_t* d = (uint8_t*)malloc(d_size);

                memcpy(label, in_buffer + sizeof(uint8_t), l_size);
                memcpy(d, in_buffer + sizeof(uint8_t) + l_size, d_size);

                I[label] = d;

                ok_response(&out, &out_len);
                gettimeofday(&end, NULL);
                total_add_time += util_time_elapsed(start, end);

                break;
            }

            case 's': {
                printf("Add image\n");

                uint8_t* label = (uint8_t*)malloc(l_size);
                uint8_t* d = (uint8_t*)malloc(d_size);

                memcpy(label, in_buffer + sizeof(uint8_t), l_size);
                memcpy(d, in_buffer + sizeof(uint8_t) + l_size, d_size);

                I[label] = d;

                ok_response(&out, &out_len);
                break;
            }

            case 'x': {


                size_t batch_size;
                /*socket_receive(readIeePipe, (unsigned char*)&batch_size, sizeof(size_t));

                for(size_t i = 0; i < batch_size; i++) {
                    void* l = malloc(10);
                    socket_receive(readIeePipe, (unsigned char*)l, 10);

                    void* d = malloc(20);
                    socket_receive(readIeePipe, (unsigned char*)d, 20);
                }*/


                break;
            }
            default:
                printf("SseServer unkonwn command: %02x\n", op);
        }
        free(in_buffer);

        // send response
        socket_send(socket, &out_len, sizeof(size_t));
        socket_send(socket, out, out_len);

        free(out);
    }

    printf("Client closed\n");
    close(socket);
}

int main(int argc, const char ** argv) {
    const int server_port = 7911;

    // register signal handler
    signal(SIGINT, close_all);

    // initialise listener socket
    int server_socket = init_server(server_port);

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
