#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>

#include "logger.h"
#include "util.h"
#include "http.h"
#include "network.h"
#include "http_parse.h"

int http_send_request(thttp_request *request, thttp_reply **replyp)
{
  unsigned char       *buf = NULL;
  size_t               buf_len = 0;
  char                *host_buf = NULL;
  char                *port_buf = NULL;
  tnetwork_driver_ctx *ctx = NULL;
  int                  ret = -1;
  tnetwork_driver_type type;

  if (asprintf(&port_buf, "%"PRIu16, http_request_port(request)) < 0) {
    logger("failed to convert port %"PRIu16" to numerical value\n",
           http_request_port(request));
    goto err;
  }

  host_buf = http_request_host(request);

  type = http_request_use_tls(request) ? NETWORK_DRIVER_TYPE_TLS : NETWORK_DRIVER_TYPE_PLAIN;
  if (! (ctx = network_driver_create(type))) {
    logger("failed to create the network driver");
    goto err;
  }

  if (network_driver_connect(ctx, host_buf, port_buf,
                             http_request_timeout(request)) < 0) {
    logger("tcp connection failed: %s:%s", host_buf, port_buf);
    goto err;
  }

  if (http_request_get_buffer(request, &buf, &buf_len) < 0) {
    logger("failed to build request buffer");
    goto err;
  }

  if (network_driver_send(ctx, buf, buf_len) < 0) {
    logger("failed to send HTTP request to %s:%s", host_buf, port_buf);
    goto err;
  }

  free(buf);
  buf = NULL;

  if (network_driver_recv(ctx, &buf, &buf_len) < 0) {
    logger("failed to receive the HTTP reply from %s:%s\n", host_buf, port_buf);
    goto err;
  }

  if (http_parse_reply(buf, buf_len, replyp) < 0) {
    fprintf(stderr, "Failed to parse HTTP reply buffer\n");
    goto err;
  }

  ret = 0;
 err:
  network_driver_free(ctx);
  free(buf);
  free(port_buf);

  return ret;
}

