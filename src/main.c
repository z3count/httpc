#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cli.h"
#include "logger.h"
#include "http.h"

int main(int argc, char **argv)
{
  char              *header_buf = NULL;
  thttp_request     *request = NULL;
  thttp_reply       *reply = NULL;
  int                rc = EXIT_FAILURE;
  struct cli_options o;

  if (cli_options_init(&o) < 0)
    goto err;

  if (cli_process_args(argc, argv, &o, &request) < 0)
    goto err;

  if (http_send_request(request, &reply) < 0) {
    logger("Failed to send HTTP request to %s:%"PRIu16"\n", o.host, o.port);
    goto err;
  }

  if (o.display.code)
    printf("%d\n", http_reply_code(reply));

  if (o.display.headers) {
    if ((header_buf = http_headers_to_string(http_reply_header(reply))))
      printf("%s\n", header_buf);
    free(header_buf);
  }

  if (o.display.body) {
    unsigned char *body = NULL;

    (void) http_reply_body(reply, &body);
    if (body)
      // Let's assume it's made of printable characters, which is generally
      // not the case, but for the sake of simplicity.  Otherwise just use
      // a kind of hexdump function, but it's not really human-enjoyable.
      printf("%s\n", (char *) body);
  }

  rc = EXIT_SUCCESS;

 err:
  cli_options_deinit(&o);
  http_reply_free(reply);
  http_request_free(request);
  return rc;
}
