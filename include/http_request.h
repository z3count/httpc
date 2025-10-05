#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include <inttypes.h>
#include <stddef.h>

#include "http_headers.h"

// We don't use anything else but GET, but hey, we do love X macros.
#define MAP(v) X(v, #v)
#define HTTP_METHOD_TABLE         \
  MAP(GET)                        \
  MAP(PUT)                        \
  MAP(CONNECT)                    \
  MAP(POST)                       \
  MAP(HEAD)                       \
  MAP(OPTIONS)                    \
  MAP(TRACE)                      \
  MAP(UNKNOWN)                    \

#define X(a, b) HTTP_METHOD_##a,
typedef enum {
    HTTP_METHOD_TABLE
} thttp_method;
#undef X

typedef struct http_request thttp_request;

void http_request_free(thttp_request *request);
int http_request_new(char *host, uint16_t port, char *path, thttp_method method, thttp_headers *headers, int use_tls, thttp_request **requestp);
int http_request_get_buffer(thttp_request *request, unsigned char **bufp, size_t *buf_lenp);
char *http_request_host(thttp_request *request);
uint16_t http_request_port(thttp_request *request);
char *http_request_path(thttp_request *request);
unsigned http_request_timeout(thttp_request *request);
int http_request_use_tls(thttp_request *request);

#endif // __HTTP_REQUEST_H__
