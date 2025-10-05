#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "util.h"
#include "utest.h"
#include "strutil.h"
#include "logger.h"
#include "cli.h"

static struct option long_options[] =
{
  {"help",        no_argument,       NULL, 'h'},
  {"version",     no_argument,       NULL, 'v'},
  {"debug",       no_argument,       NULL, 'D'},
  {"test",        no_argument,       NULL, 't'},
  {"tls",         no_argument,       NULL, 'T'},
  {"host",        required_argument, NULL, 'H'},
  {"port",        required_argument, NULL, 'p'},
  {"path",        required_argument, NULL, 'P'},
  {"http-header", required_argument, NULL,  0},
  {"get-code",    no_argument,       NULL,  0},
  {"get-headers", no_argument,       NULL,  0},
  {"get-body",    no_argument,       NULL,  0},
  {NULL,          0,                 NULL,  0},
};

static char *progname = "httpc";
static char *progversion = "0.1";
       
static void cli_usage(const char * const progname)
{
  fprintf(stderr, "%s %s\n"
          "This HTTP (no TLS) client performs basic GET requests.\n"
          "\n"
          "\t-h, --help\t\t           display this message\n"
          "\t-v, --version\t\t        display the version\n"
          "\t-D, --debug\t\t          enable debug mode (deactivated)\n"
          "\t-t, --test\t\t           run the unit tests\n"
          "\t-T, --tls\t\t            enable TLS\n"
          "\t-H, --host <host>\t\t    set the target hostname\n"
          "\t-p, --port <port>\t\t    set the target port/tcp\n"
          "\t-P, --path <path>\t\t    specify the http path\n"
          "\t    --http-header <hdr>  set a given key:value header\n"
          "\t    --get-code\t\t       display the reply code\n"
          "\t    --get-headers\t\t    display the reply headers\n"
          "\t    --get-body\t\t       display the reply body\n"
          "\n",
          progname, progversion);

  exit(EXIT_SUCCESS);
}

// Incomplete: we should handle cases with several ':', etc.
static int cli_parse_header(char *input, char **keyp, char **valuep)
{
  char *key = NULL;
  char *value = NULL;
  char *p = NULL;
  char *buf = NULL;
  int   rc = -1;

  if (! input)
    goto err;

  if (! (buf = strdup(input))) {
    logger("strdup: %m");
    goto err;
  }

  if (! (p = strchr(buf, ':'))) {
    logger("missing header colon-separator: %s", input);
    goto err;
  }

  *p = 0;

  if (! p[1]) // The last character is ':'
    goto err;

  if (trim(buf, &key) < 0 || ! strlen(key))
    goto err;

  if (trim(p + 1, &value) < 0 || ! strlen(value))
    goto err;

  if (keyp)
    *keyp = key;
  else
    free(key);

  if (valuep)
    *valuep = value;
  else
    free(value);

  free(buf);
  return 0;

 err:
  free(buf);
  free(key);
  free(value);
  return rc;
}

int cli_process_args(int argc, char **argv, struct cli_options *options, thttp_request **requestp)
{
  int            ch;
  int            option_index;
  thttp_request *request = NULL;
  char          *name = NULL;

  while ((ch = getopt_long(argc, argv, "hvdtTH:p:P:", long_options, &option_index)) != -1) {
    switch (ch) {
    case 0: // long options
      name = (char *) long_options[option_index].name;
      if (! strcmp(name, "get-code")) {
        options->display.code = 1;
      } else if (! strcmp(name, "get-body")) {
        options->display.body = 1;
      } else if (! strcmp(name, "get-headers")) {
        options->display.headers = 1;
      } else if (! strcmp(name, "http-header")) {
        char *key = NULL;
        char *value = NULL;

        if (cli_parse_header(optarg, &key, &value) < 0) {
          logger("failed to parse the provided header: %s", optarg);
          goto err;
        }

        int rc = http_headers_add(options->headers, key, value);
        free(key);
        free(value);
        if (rc < 0) {
          logger("failed to add new header %s", optarg);
          goto err;
        }
      } else {
        // Unknown
      }
      break;

    case 't':
      exit(utest_run());

    case 'v':
    case 'h':
      cli_usage(progname);
      break;

    case 'D':
      break;

    case 'T':
      options->use_tls = 1;
      break;

    case 'p': {
      char *endptr;
      unsigned long port = strtoul(optarg, &endptr, 10);
      if (endptr == optarg) {
        logger("strtoul(%s): %m", optarg);
        goto err;
      }
      options->port = (uint16_t) port;
    }
      break;

    case 'H':
      free(options->host);
      if (! (options->host = strdup(optarg))) {
        logger("strdup: %m");
        goto err;
      }

      if (http_headers_update_value(options->headers, "Host", optarg) < 0) {
        logger("unable to update the 'Host' header value");
        goto err;
      }
      break;

    case 'P':
      free(options->path);
      if (! (options->path = strdup(optarg))) {
        logger("strdup: %m");
        goto err;
      }
      break;
    }
  }

  if (http_request_new(options->host, options->port, options->path,
                       options->method, options->headers, options->use_tls, &request) < 0) {
    logger("Failed to create a new HTTP request on %s:%"PRIu16"\n",
           options->host, options->port);
    goto err;
  }

  if (requestp)
    *requestp = request;
  else
    http_request_free(request);

  return 0;
 err:
  return -1;
}

int cli_options_init(struct cli_options *options)
{
  memset(options, 0, sizeof *options);

  // The "Host: " header is mandatory. By default use the same value as the
  // HTTP server we're requesting.  We can supersed this value thanks to
  // the --host CLI option.
  thttp_headers *headers = NULL;
  if (http_headers_new("Host", DEFAULT_HOST, &headers) < 0)
    return -1;

  if (! (options->host = strdup(DEFAULT_HOST))) {
    http_headers_free(headers);
    logger("strdup: %m");
    return -1;
  }

  if (! (options->path = strdup(DEFAULT_PATH))) {
    http_headers_free(headers);
    logger("strdup: %m");
    return -1;
  }

  options->port = DEFAULT_PORT;
  options->use_tls = DEFAULT_USE_TLS;
  options->headers = headers;
  options->method = DEFAULT_METHOD;
  options->display.code = 0;
  options->display.headers = 0;
  options->display.body = 0;

  return 0;
}

void cli_options_deinit(struct cli_options *options)
{
  free(options->host);
  free(options->path);
}


//
// Unit tests
//

#include "../tests/cli_utest.c"
