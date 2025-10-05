#ifndef __CLI_H__
#define __CLI_H__

#include <stdio.h>
#include <inttypes.h>

#include "http_headers.h"
#include "http_request.h"

struct cli_options {
#define DEFAULT_HOST "httpbin.io"
  char          *host;
#define DEFAULT_PORT 80
  uint16_t       port;
#define DEFAULT_PATH "/get"
  char          *path;
#define DEFAULT_METHOD HTTP_METHOD_GET 
  thttp_method   method;
  thttp_headers *headers;
#define DEFAULT_USE_TLS 0
  int            use_tls;

  struct {
    int code;
    int headers;
    int body;
  } display;
};

int cli_options_init(struct cli_options *options);
void cli_options_deinit(struct cli_options *options);
int cli_process_args(int argc, char **argv, struct cli_options *optionsp, thttp_request **requestp);

// Unit tests.
int cli_utest(void);

#endif // __CLI_H__
