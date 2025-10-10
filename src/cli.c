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
  {"http-header", required_argument, NULL,  0},
  {"get-code",    no_argument,       NULL,  0},
  {"get-headers", no_argument,       NULL,  0},
  {"get-body",    no_argument,       NULL,  0},
  {NULL,          0,                 NULL,  0},
};

static char *progname = "httpc";
static char *progversion = "0.1";

static void cli_version(const char * const progname)
{
  fprintf(stderr, "%s %s\n", progname, progversion);
}

static void cli_usage(const char * const progname)
{
  cli_version(progname);
  fprintf(stderr,
          "This HTTP client performs basic GET requests.\n\n"
	  "%s TARGET [OPTIONS...]\n"
          "\n"
          "\t-h, --help\t\t           display this message\n"
          "\t-v, --version\t\t        display the version\n"
          "\t-D, --debug\t\t          enable debug mode (deactivated)\n"
          "\t-t, --test\t\t           run the unit tests\n"
          "\t    --http-header <hdr>  set a given key:value header\n"
          "\t    --get-code\t\t       display the reply code\n"
          "\t    --get-headers\t\t    display the reply headers\n"
          "\t    --get-body\t\t       display the reply body\n"
          "\n",
          progname);
}


// Target: [<scheme>://]hostname[:port][/path]
// With 'scheme', 'port' and 'path' being optional.
//
// NOTE: we do not validate the hostname format
static int cli_parse_target(char *target, int *is_tlsp, char **hostnamep, uint16_t *portp, char **pathp)
{
  char  *hostname = NULL;
  int    port = -1;
  char  *path = NULL;
  int    is_tls = 0;
  char  *p = NULL;
  char  *slash = NULL;
  char  *colon = NULL;
  size_t target_len = 0;
  char   *target_backup = NULL;

#define HTTP "http://"
#define HTTPS "https://"

  if (! target) {
    logger("invalid format, NULL target");
    goto err;
  }

  target_len = strlen(target);
  if (! target_len) {
    logger("invalid format, empty target");
    goto err;
  }

  if (! (p = strdup(target))) {
    logger("strdup(): %m");
    goto err;
  }

  target_backup = p;

  if (0 == strncmp(p, HTTPS, sizeof HTTPS - 1)) {
    is_tls = 1;
    p += sizeof HTTPS - 1;
  } else if (0 == strncmp(p, HTTP, sizeof HTTP - 1)) {
    p += sizeof HTTP - 1;
  }

  if (! p || ! *p) {
    logger("invalid format, hostname is missing");
    goto err;
  }

  // Don't look for the first ':' because it might appear in the path, so first
  // make sure we're before the first '/', if any

  if ((slash = strchr(p, '/'))) {
    if (! (path = strdup(slash))) {
      logger("strdup: %m");
      goto err;
    }
    *slash = 0;
  }

  if ((colon = strchr(p, ':'))) {
    if ((colon + 1) && ! *(colon + 1)) {
      logger("invalid format, port is missing: %s", target);
      goto err;
    }

    *colon++ = 0;

    char *endptr = NULL;
    port = (int) strtoul(colon, &endptr, 10);
    if (endptr == colon) {
      logger("invalid format, erronous port number: %s", colon);
      goto err;
    }

    if (port <= 0 || port >= UINT16_MAX) {
      logger("invalid format, port out of range: %s", colon);
      goto err;
    }

    *colon = 0;
  }

  if (! *p) {
    logger("invalid format, empty hostname: %s", target);
    goto err;
  }

  // So here, we either put a nul-terminator in place of ':', or there's
  // no specified port. We can safely duplicate the hostname.
  if (! (hostname = strdup(p))) {
    logger("strdup: %m");
    goto err;
  }

  free(target_backup);

  if (is_tlsp)
    *is_tlsp = is_tls;

  if (hostnamep)
    *hostnamep = hostname;
  else
    free(hostname);

  if (portp) {
    if (port > 0) {
      *portp = (uint16_t) port;
    } else if (is_tls) {
      *portp = 443;
    } else {
      *portp = 80;
    }
  }

  if (pathp)
    *pathp = path;
  else
    free(path);

  return 0;

 err:
  free(target_backup);
  free(hostname);
  free(path);
  return -1;
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
  char          *host = NULL;
  char          *path = NULL;

  if (argc < 2) {
    cli_usage(progname);
    exit(EXIT_FAILURE);
  }

  if (cli_parse_target(argv[1], &options->use_tls, &host, &options->port, &path) < 0) {
    logger("failed to parse target '%s'", argv[1]);
    goto err;
  }

  if (host) {
    free(options->host);
    options->host = host;
  }

  if (path) {
    free(options->path);
    options->path = path;
  }
    

  while ((ch = getopt_long(argc, argv, "hvdt", long_options, &option_index)) != -1) {
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
      cli_version(progname);
      exit(EXIT_SUCCESS);
    case 'h':
      cli_usage(progname);
      exit(EXIT_SUCCESS);
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
