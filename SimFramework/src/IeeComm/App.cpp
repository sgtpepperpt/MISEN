#include "App.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include "untrusted_util.h"

// to pass data to client thread
typedef struct client_data {
    int socket;
} client_data;

static int server_socket;

static void close_all(int signum) {
    close(server_socket);
    fflush(NULL);
    exit(0);
}

void* process_client(void* args) {
    const int socket = ((client_data *)args)->socket;
    free(args);

    while (1) {
        size_t in_len;
        untrusted_util::socket_receive(socket, &in_len, sizeof(size_t));

        void *in_buffer = malloc(in_len);
        untrusted_util::socket_receive(socket, in_buffer, in_len);

        void* out;
        size_t out_len;

        // execute ecall
        ecall_process(&out, &out_len, in_buffer, in_len);
        free(in_buffer);

        // send to client
        untrusted_util::socket_send(socket, &out_len, sizeof(size_t));
        untrusted_util::socket_send(socket, out, out_len);

        free(out);
    }

    printf("Client closed\n");
    close(socket);
}

int main(int argc, const char **argv) {
    // port to start the server on
    const int server_port = IEE_PORT;

    // nr of threads in thread pool
    const unsigned nr_threads = 3;

    // register signal handler
    signal(SIGINT, close_all);

    ecall_init_enclave(nr_threads);

    // initialise thread pool
    thread_pool* pool = init_thread_pool(nr_threads);

    // initialise listener socket
    server_socket = untrusted_util::init_server(server_port);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = 0;

    printf("Listening for requests...\n");
    while (1) {
        client_data* data = (client_data*)malloc(sizeof(client_data));

        if ((data->socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
            printf("Accept failed!\n");
            //exit(1);
            break;
        }

        printf("Client connected (%s)\n", inet_ntoa(client_addr.sin_addr));

        pthread_t tid;
        pthread_create(&tid, NULL, process_client, data);
    }

    close(server_socket);

    // cleanup and exit
    delete_thread_pool(pool);

    printf("Enclave successfully terminated.\n");
    return 0;
}
