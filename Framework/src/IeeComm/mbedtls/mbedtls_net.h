#ifndef _MBEDTLS_NET_H_
#define _MBEDTLS_NET_H_
/*
void mbedtls_net_init(mbedtls_net_context* ctx);
int mbedtls_net_connect(mbedtls_net_context* ctx, const char* host, const char* port, int proto);
int mbedtls_net_bind(mbedtls_net_context* ctx, const char* bind_ip, const char* port, int proto);
static int net_would_block(const mbedtls_net_context* ctx);
int mbedtls_net_accept(mbedtls_net_context* bind_ctx, mbedtls_net_context* client_ctx, void* client_ip, size_t buf_size, size_t* ip_len);
int mbedtls_net_set_block(mbedtls_net_context* ctx);
int mbedtls_net_set_nonblock(mbedtls_net_context* ctx);
void mbedtls_net_free(mbedtls_net_context* ctx);
*/
#endif
