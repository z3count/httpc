#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include <stddef.h>

#include "http_headers.h"

typedef struct http_reply thttp_reply;

void http_reply_free(thttp_reply *reply);
int http_reply_new(int code, thttp_headers *headers, unsigned char *body, size_t body_len, thttp_reply **replyp);
int http_reply_code(thttp_reply * reply);
thttp_headers *http_reply_header(thttp_reply *reply);
size_t http_reply_body(thttp_reply *reply, unsigned char **bodyp);

#endif // __HTTP_RESPONSE_H__
