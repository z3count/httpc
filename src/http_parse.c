#define _POSIX_C_SOURCE 200809L // strndup()
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "util.h"
#include "logger.h"
#include "strutil.h"
#include "http_parse.h"

static int http_parse_code(unsigned char *udata, size_t data_size, int *codep)
{
  int   code = 0;
  char  http_resp[] = "HTTP/1.";
  char *endptr = NULL;
  char *p = NULL;
  char *data = (char *) udata;
  
  // Sanity check
  if (! data || ! data_size) {
    goto err;
  }

  while (data && *data && isspace(*data))
    data++;
  
  if (strncasecmp(http_resp, (char *) data, sizeof http_resp - 1))
    goto err;

  p = data + sizeof http_resp - 1;

  // Skip the http version
  while (p && *p && ! isspace(*p))
    p++;

  // Skip spaces
  while (p && *p && isspace(*p))
    p++;

  if (! p || ! *p)
    goto err;

  code = (int) strtol(p, &endptr, 0);
  if (endptr && ('\0' == *endptr || (char *) p == endptr))
    goto err;
  
  if (codep)
    *codep = code;

  return 0;
 err:
  return -1;
}

// At least make sure we have the CRLF CRLF header-body separator, return the
// header total length (without the last separator)
static int http_parse_sanity_check(char *buf, size_t buf_size)
{
  char *sep = NULL;
  (void) buf_size;

  if (! (sep = strstr(buf, CRLF CRLF))) {
    logger("missing the CRLF CRLF header-body separator: %s", buf);
    goto err;
  }

  return (int) PTRDIFF(sep, buf);
 err:
  return -1;
}

static int http_parse_headers(unsigned char *buf, size_t buf_size, thttp_headers **headersp)
{
  char          *headers_buf = NULL;
  char          *p = NULL;
  thttp_headers *headers = NULL;
  int            headers_len = -1;
  char          *nkey = NULL;
  char          *nvalue = NULL;

  if (! buf)
    goto err;

  if ((headers_len = http_parse_sanity_check((char *) buf, buf_size)) < 0)
    goto err;

  if (! (headers_buf = malloc(headers_len + CRLF_LEN + 1))) {
    logger("malloc: %m");
    goto err;
  }

  memcpy(headers_buf, buf, headers_len + CRLF_LEN);
  headers_buf[headers_len + CRLF_LEN] = '\0';

  // Ignore the first line: HTTP/1.x <Code> <Reason>\r\n
  if (! (p = strstr(headers_buf, CRLF))) {
    logger("missing the very first CRLF separator: %s", headers_buf);
    goto err;
  }

  p += CRLF_LEN; // Ok we're at the beginning of the headers

  while (p < &headers_buf[headers_len - CRLF_LEN]) {
    char *colon = NULL;
    char *eol = NULL;

    if (! (eol = strstr(p, CRLF)))
      break;

    // Did we reach the end of headers?
    if (eol == p)
      break;

    // Only consider this header line: "Key: Value"
    *eol = '\0';

    if (! (colon = strchr(p, ':'))) {
      // Don't return the function, simply ignore this line.
      logger("invalid header line: %s", p);
      goto err;
    }

    *colon = '\0';
    if (trim(p, &nkey) <  0 ||
        (trim(colon + 1, &nvalue) < 0)) {
      logger("failed to extract key or value: %s", p);
      free(nkey);
      free(nvalue);
      continue;
    }

    if (! headers) {
      if (http_headers_new(nkey, nvalue, &headers) < 0) {
        logger("failed to create a new HTTP header");
        free(nkey);
        free(nvalue);
        goto err;
      }
    } else if (http_headers_add(headers, nkey, nvalue) < 0) {
      logger("failed to add a new key/value to the headers");
      free(nkey);
      free(nvalue);
      goto err;
    }
    
    p = eol + CRLF_LEN;
    free(nvalue);
    free(nkey);
  }

  free(headers_buf);

  if (headersp)
    *headersp = headers;
  else
    http_headers_free(headers);

  return 0;

 err:
  free(headers_buf);
  http_headers_free(headers);
  return -1;
}

// Just look for the CRLF CRLF then dump the remaining data
static int http_parse_body(unsigned char *buf, size_t buf_size, unsigned char **bodyp, size_t *body_lenp)
{
  unsigned char *body = NULL;
  size_t         body_len = 0;
  char          *p = NULL;

  (void) buf_size;

  if (! buf) {
    logger("must provide a non-NULL body");
    goto err;
  }

  if (NULL == (p = strstr((char *) buf, CRLF CRLF))) {
    logger("unable to locate the CRLF CRLF header/body delimiter");
    goto err;
  }

  p += CRLF_LEN * 2;

  size_t difference = (size_t) PTRDIFF(p, buf);
  body_len = buf_size - difference;
  if (NULL == (body = malloc(body_len))) {
    logger("malloc: %m");
    goto err;
  }

  memcpy(body, p, body_len);
  body[body_len - 1] = 0;

  if (bodyp)
    *bodyp = body;
  else
    free(body);

  if (body_lenp)
    *body_lenp = body_len;

  return 0;
 err:
  free(body);
  return -1;
}

int http_parse_reply(unsigned char *buf, size_t buf_len, thttp_reply **replyp)
{
  unsigned char  *body = NULL;
  size_t          body_len = 0;
  thttp_headers  *headers = NULL;
  thttp_reply *reply = NULL;
  int             code = -1;

  if (! buf) {
    logger("Got an empty reply to parse");
    goto err;
  }

  if (http_parse_code(buf, buf_len, &code) < 0) {
    logger("failed to extract code from %s", buf);
    goto err;
  }

  if (http_parse_headers(buf, buf_len, &headers) < 0)
    goto err;

  if (http_parse_body(buf, buf_len, &body, &body_len) < 0)
    goto err;

  if (http_reply_new(code, headers, body, body_len, &reply) < 0)
    goto err;

  if (replyp)
    *replyp = reply;
  else
    http_reply_free(reply);

  return 0;

 err:
  http_reply_free(reply);
  return -1;
}


//
// UNIT TESTS
//

#include "../tests/http_parse_utest.c"
