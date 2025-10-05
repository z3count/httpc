#ifndef __HTTP_H__
#define __HTTP_H__

#include "http_headers.h"
#include "http_request.h"
#include "http_reply.h"


char *http_method_to_str(thttp_method method);
int http_send_request(thttp_request *request, thttp_reply **replyp);

#endif // __HTTP_H__
