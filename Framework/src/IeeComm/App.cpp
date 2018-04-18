#include "App.h"

// to pass data to client thread
typedef struct client_data {
    sgx_enclave_id_t eid;
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
    sgx_enclave_id_t eid = ((client_data *)args)->eid;
    free(args);

    while (1) {
        size_t in_len;
        socket_receive(socket, &in_len, sizeof(size_t));

        void *in_buffer = malloc(in_len);
        socket_receive(socket, in_buffer, in_len);

        void* out;
        size_t out_len;

        // execute ecall
        sgx_status_t status = ecall_process(eid, &out, &out_len, in_buffer, in_len);
        free(in_buffer);

        // verify return values from sgx
        if (status != SGX_SUCCESS) {
            print_error_message(status);
            exit(-1);
        }

        // send to client
        socket_send(socket, &out_len, sizeof(size_t));
        socket_send(socket, out, out_len);

        free(out);
    }

    printf("Client closed\n");
    close(socket);
}

int SGX_CDECL main(int argc, const char **argv) {
    // port to start the server on
    const int server_port = 7910;

    // nr of threads in enclave
    const unsigned nr_threads = 4;

    // register signal handler
    signal(SIGINT, close_all);

    // initialise the enclave
    sgx_enclave_id_t eid = 0;
    if (initialise_enclave(&eid) < 0) {
        printf("Problem with enclave initialisation, exiting...\n");
        return -1;
    }

    printf("Enclave id: %lu\n", eid);

    // perform internal enclave initialisation
    sgx_status_t status = ecall_init_enclave(eid);
    if (status != SGX_SUCCESS) {
        print_error_message(status);
        exit(-1);
    }

    // initialise thread pool
    thread_pool* pool = init_thread_pool(eid, nr_threads);

    // initialise listener socket
    server_socket = init_server(server_port);

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = 0;

    printf("Listening for requests...\n");
    while (1) {
        client_data *data = (client_data *)malloc(sizeof(client_data));
        data->eid = eid;

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
    destroy_enclave(eid);
    delete_thread_pool(pool);

    printf("Enclave successfully terminated.\n");
    return 0;
}
