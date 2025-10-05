#define _XOPEN_SOURCE >= 500 // strdup()
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "logger.h"
#include "http_request.h"
#include "http_headers.h"

#define DEFAULT_TIMEOUT_SEC 5

#define X(a, b) {HTTP_METHOD_##a, b},
struct method_mapping {
  thttp_method method;
  char        *str;
} http_method_mapping[] = {
  HTTP_METHOD_TABLE
};
#undef X
#undef MAP

struct http_request {
  int            use_tls;
  char          *host;
  uint16_t       port;
  char          *path;
  unsigned       timeout_sec;
  thttp_method   method;
  thttp_headers *headers;
};

int http_request_use_tls(thttp_request *request)
{
  return request->use_tls;
}

char *http_request_host(thttp_request *request)
{
  return request->host;
}

uint16_t http_request_port(thttp_request *request)
{
  return request->port;
}

char *http_request_path(thttp_request *request)
{
  return request->path;
}

unsigned http_request_timeout(thttp_request *request)
{
  return request->timeout_sec;
}

static char *http_method_to_str(thttp_method method)
{
  if (method < 0 || method >= HTTP_METHOD_UNKNOWN) {
    return http_method_mapping[HTTP_METHOD_UNKNOWN].str;
  }

  return http_method_mapping[method].str;
}

void http_request_free(thttp_request *request)
{
  if (request) {
    free(request->host);
    free(request->path);
    http_headers_free(request->headers);
  }

  free(request);
}

static int http_request_new_internal(thttp_request **requestp)
{
  thttp_request *request = malloc(sizeof *request);
  if (! request)
    goto err;

  request->host = NULL;
  request->port = 0;
  request->path = NULL;
  request->method = HTTP_METHOD_UNKNOWN;
  request->headers = NULL;

  if (requestp)
    *requestp = request;
  else
    http_request_free(request);

  return 0;
 err:
  return -1;
}

int http_request_new(char *host, uint16_t port, char *path, thttp_method method, thttp_headers *headers, int use_tls, thttp_request **requestp)
{
  thttp_request *request;

  if (http_request_new_internal(&request) < 0)
    goto err;

  request->host = strdup(host);
  request->path = strdup(path);
  if (! request->host || ! request->path)
    goto err;

  request->use_tls = use_tls;
  request->port = port;
  request->method = method;
  request->headers = headers;
  request->timeout_sec = DEFAULT_TIMEOUT_SEC;

  if (requestp)
    *requestp = request;
  else
    http_request_free(request);

  return 0;
 err:
  return -1;
}

// We might want to send raw data, not a string
int http_request_get_buffer(thttp_request *request, unsigned char **bufp, size_t *buf_lenp)
{
  unsigned char *buf = NULL;
  char          *header_buf = NULL;
  int            n = -1;

  header_buf = http_headers_to_string(request->headers);

  n = asprintf((char **) &buf, "%s %s HTTP/1.1\r\n%s\r\n",
               http_method_to_str(request->method),
               request->path,
               header_buf ? header_buf : "");
  
  free(header_buf);
  if (n < 0)
    goto err;

  if (bufp)
    *bufp = buf;
  else
    free(buf);

  if (buf_lenp)
    *buf_lenp = (size_t) n + 1;

  return 0;
 err:
  free(buf);
  return -1;
}
