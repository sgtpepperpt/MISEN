#include "Client.h"

#include "untrusted_util.h"
#include "untrusted_util_tls.h"
#include "definitions.h"

#include <getopt.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sodium.h>

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#include "mbedtls/certs.h"

#include "ImageSearch.h"

using namespace cv;
using namespace cv::xfeatures2d;
using namespace std;

static void my_debug(void* ctx, int level, const char* file, int line, const char* str) {
    ((void)level);
    fprintf((FILE*)ctx, "%s:%04d: %s", file, line, str);
    fflush((FILE*)ctx);
}

int main(int argc, char** argv) {
    // static params
    const char* server_name = IEE_HOSTNAME;
    const int server_port = IEE_PORT;
    const size_t desc_len = 64;
    const double surf_threshold = 400;

    // may be specified by user
    size_t nr_clusters = 1000;
    int train_only = 0;
    int load_clusters = 0;
    int do_search_test = 0;
    int add_all = 0;

    int load_uee = 0;
    int write_uee = 0;

    // parse terminal arguments
    int c;
    while ((c = getopt(argc, argv, "htlsarwk:")) != -1) {
        switch (c) {
            case 'k':
                nr_clusters = std::stoul(optarg);
                break;
            case 't':
                train_only = 1;
                break;
            case 'h':
                printf("Usage: ./Client nr-imgs\n");
                exit(0);
            case 'l':
                load_clusters = 1;
                break;
            case 's':
                do_search_test = 1;
                break;
            case 'a':
                add_all = 1;
                break;
            case 'r':
                load_uee = 1;
                break;
            case 'w':
                write_uee = 1;
                break;
            case '?':
                if (optopt == 'c')
                    fprintf(stderr, "-%c requires an argument.\n", optopt);
                else if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                exit(1);
            default:
                exit(-1);
        }
    }

    if (argv[optind] && (add_all || load_uee)) {
        printf("Nr of images can not be specified with add all or load uee!\n");
        exit(1);
    }

    if (!argv[optind] && !load_uee && !add_all) {
        printf("Nr of images not specified!\n");
        exit(1);
    }

    if (load_uee && add_all) {
        printf("Add all images is not compatible with load uee option!\n");
        exit(1);
    }

    if (load_uee && write_uee) {
        printf("Load UEE is not compatible with write UEE option!\n");
        exit(1);
    }

    const int nr_images = add_all || load_uee ? -1 : atoi(argv[optind]);

    printf("Running with %d images, %lu clusters\n", nr_images, nr_clusters);

    // init mbedtls
    // put port in char buf
    char port[5];
    sprintf(port, "%d", server_port);

    int ret;
    mbedtls_net_context server_fd;
    uint32_t flags;

    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt cacert;

    // initialise the RNG and the session data
    mbedtls_net_init(&server_fd);
    mbedtls_ssl_init(&ssl);
    mbedtls_ssl_config_init(&conf);
    mbedtls_x509_crt_init(&cacert);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    printf("seeding the random number generator...\n");
    mbedtls_entropy_init(&entropy);
    const char* pers = "ssl_client1";
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char*)pers, strlen(pers))) != 0) {
        printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        exit(1);
    }

    // initialise certificates
    printf("loading the CA root certificate...\n");
    ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char*)mbedtls_test_cas_pem, mbedtls_test_cas_pem_len);
    if (ret < 0) {
        printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
        exit(1);
    }

    printf("ok (%d skipped)\n", ret);

    // start connection
    printf("connecting to tcp/%s/%s...\n", server_name, port);
    if ((ret = mbedtls_net_connect(&server_fd, server_name, port, MBEDTLS_NET_PROTO_TCP)) != 0) {
        printf(" failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
        exit(1);
    }

    printf("setting up the SSL/TLS structure...\n");
    if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
        exit(1);
    }

    /* OPTIONAL is not optimal for security,
     * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
    mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
        printf("failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
        exit(1);
    }

    if ((ret = mbedtls_ssl_set_hostname(&ssl, server_name)) != 0) {
        printf("failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
        exit(1);
    }

    mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    printf("performing the SSL/TLS handshake...\n");
    while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            printf("failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
            exit(1);
        }
    }

    // verify the server certificate
    printf("verifying peer X.509 certificate...\n");

    // in real life, we probably want to bail out when ret != 0
    if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0) {
        printf("failed\n");

        char vrfy_buf[512];
        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "! ", flags);
        printf("%s\n", vrfy_buf);
    }

    struct timeval start;
    gettimeofday(&start, NULL);

    // image descriptor parameters
    Ptr<SURF> surf = SURF::create(surf_threshold);
    Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("FlannBased");
    //BOWImgDescriptorExtractor* bowExtractor = new BOWImgDescriptorExtractor(surf, matcher);

    const vector<string> files = get_filenames(nr_images);

    // init iee and server
    size_t in_len = 0;
    uint8_t* in = NULL;

    init(&in, &in_len, nr_clusters, desc_len);
    iee_comm(&ssl, in, in_len);
    free(in);

    if (!load_clusters) {
        // adding
        for (unsigned i = 0; i < files.size(); i++) {
            printf("Train img (%u/%lu) %s\n", i, files.size(), files[i].c_str());
            add_train_images(&in, &in_len, surf, files[i]);
            iee_comm(&ssl, in, in_len);
            free(in);
            printf("\n");
        }

        // train
        train(&in, &in_len);
        iee_comm(&ssl, in, in_len);
        free(in);
    } else {
        train_load_clusters(&in, &in_len);
        iee_comm(&ssl, in, in_len);
        free(in);
    }

    struct timeval end;
    gettimeofday(&end, NULL);
    printf("-- Train elapsed time: %ldms --\n", untrusted_util::time_elapsed_ms(start, end));

    if (train_only)
        return 0;

    gettimeofday(&start, NULL);
    if (load_uee) {
        // tell iee to load images from disc
        unsigned char op = OP_IEE_READ_MAP;
        iee_comm(&ssl, &op, 1);
    } else {
        // add images to repository
        for (unsigned i = 0; i < files.size(); i++) {
            printf("Add img (%u/%lu)\n\n", i, files.size());
            add_images(&in, &in_len, surf, files[i]);
            iee_comm(&ssl, in, in_len);
            free(in);
        }
    }

    gettimeofday(&end, NULL);
    printf("-- Add img elapsed time: %ldms %lu imgs--\n", untrusted_util::time_elapsed_ms(start, end), files.size());

    if (write_uee) {
        // send persist message to iee
        unsigned char op = OP_IEE_WRITE_MAP;
        iee_comm(&ssl, &op, 1);
    }

    // search
    if (do_search_test) {
        search_test(&ssl, surf);
    } else {
        //TODO perform a search from args?
        gettimeofday(&start, NULL);

        search(&in, &in_len, surf, get_filenames(10)[0]);
        iee_send(&ssl, in, in_len);
        free(in);

        // receive response and print
        size_t res_len;
        uint8_t* res;
        iee_recv(&ssl, &res, &res_len);

        unsigned nr;
        memcpy(&nr, res, sizeof(unsigned));
        printf("nr of imgs: %u\n", nr);

        for (unsigned j = 0; j < nr; ++j) {
            unsigned long id;
            memcpy(&id, res + sizeof(size_t) + j * SRC_RES_LEN, sizeof(unsigned long));

            double score;
            memcpy(&score, res + sizeof(size_t) + j * SRC_RES_LEN + sizeof(unsigned long), sizeof(double));

            printf("%lu %f\n", id, score);

        }

        gettimeofday(&end, NULL);
        printf("-- Search elapsed time: %ldms--\n", untrusted_util::time_elapsed_ms(start, end));
    }

    // clear
    clear(&in, &in_len);
    iee_comm(&ssl, in, in_len);
    free(in);

    gettimeofday(&end, NULL);
    printf("Total elapsed time: %ldms\n", untrusted_util::time_elapsed_ms(start, end));

    // close ssl connection
    mbedtls_ssl_close_notify( &ssl );

    // ssl cleanup
    mbedtls_net_free(&server_fd);
    mbedtls_ssl_free(&ssl);
    mbedtls_ssl_config_free(&conf);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    return 0;
}
