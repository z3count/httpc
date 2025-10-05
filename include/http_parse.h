#ifndef __HTTP_PARSE_H__
#define __HTTP_PARSE_H__

#include "http_reply.h"

int http_parse_reply(unsigned char *buf, size_t buf_len, thttp_reply **replyp);

// Unit test.
int http_parse_utest(void);

#endif // __HTTP_PARSE_H__
