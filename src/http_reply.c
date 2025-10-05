#include <stdlib.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>

#include "util.h"
#include "logger.h"
#include "http_reply.h"

struct http_reply {
  unsigned int   code;
  thttp_headers *headers;
  unsigned char *body;
  size_t         body_len;
};

void http_reply_free(thttp_reply *reply)
{
  if (reply) {
    free(reply->body);
    http_headers_free(reply->headers);
  }

  free(reply);
}

static int http_reply_new_internal(thttp_reply **replyp)
{
  thttp_reply *reply = malloc(sizeof *reply);
  if (! reply)
    goto err;

  reply->code = 0;
  reply->headers = NULL;
  reply->body = NULL;
  reply->body_len = 0;

  if (replyp)
    *replyp = reply;
  else
    http_reply_free(reply);

  return 0;
 err:
  return -1;
}

int http_reply_new(int code, thttp_headers *headers, unsigned char *body, size_t body_len, thttp_reply **replyp)
{
  thttp_reply *reply;

  if (http_reply_new_internal(&reply) < 0)
    goto err;

  reply->code = code;
  reply->headers = headers;
  reply->body = body;
  reply->body_len = body_len;

  if (replyp)
    *replyp = reply;
  else
    http_reply_free(reply);

  return 0;
 err:
  return -1;
}

int http_reply_code(thttp_reply *reply)
{
  return reply->code;
}

thttp_headers *http_reply_header(thttp_reply *reply)
{
  return reply->headers;
}

size_t http_reply_body(thttp_reply *reply, unsigned char **bodyp)
{
  *bodyp = reply->body;
  return reply->body_len;
}
