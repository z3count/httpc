#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "logger.h"
#include "strutil.h"
#include "http_headers.h"

struct http_headers {
  struct http_headers_elem *elems;
  unsigned n_elems;
};

void http_headers_free(thttp_headers *headers)
{
  if (! headers)
    return;

  for (unsigned i = 0; i < headers->n_elems; i++) {
    free(headers->elems[i].key);
    free(headers->elems[i].value);
  }

  free(headers->elems);
  free(headers);
}

static int http_headers_new_internal(thttp_headers **headersp)
{
  thttp_headers *headers = malloc(sizeof *headers);
  if (! headers) {
    logger("malloc: %m");
    goto err;
  }

  headers->elems = NULL;
  headers->n_elems = 0;

  if (headersp)
    *headersp = headers;
  else
    http_headers_free(headers);
  
  return 0;
 err:
  return -1;
}

int http_headers_new(char *key, char *value, thttp_headers **headersp)
{
  struct http_headers *headers;

  // We accept empty strings as key/value but not NULL
  if (! key || ! value) {
    logger("both key and value must be non-NULL");
    goto err;
  }

  if (http_headers_new_internal(&headers) < 0)
    goto err;

  if (! (headers->elems = malloc(sizeof *headers->elems))) {
    logger("malloc: %m");
    goto free_header_err;
  }

  char *nkey = strdup(key);
  char *nvalue = strdup(value);
  if (! nkey || ! nvalue) {
    logger("strdup: %m");
    free(nkey);
    free(nvalue);
    goto free_header_err;
  }

  headers->elems[0].key = nkey;
  headers->elems[0].value = nvalue;
  headers->n_elems = 1;

  if (headersp)
    *headersp = headers;
  else
    http_headers_free(headers);
  
  return 0;

 free_header_err:
  http_headers_free(headers);
 err:
  return -1;
}

// key and value are already allocated, just to pointer assignment here.
int http_headers_add(thttp_headers *headers, char *key, char *value)
{
#define HTTP_HEADER_MAX_ENTRIES 256
  unsigned                  n = 0;
  struct http_headers_elem *tmp = NULL;

  // Ok, just append it a the end.
  n = headers->n_elems + 1;

  if (n > HTTP_HEADER_MAX_ENTRIES) {
    logger("we reached the maximum number of header keys: %d", n);
    goto err;
  }

  char *nvalue = strdup(value);
  char *nkey = strdup(key);
  if (! nvalue || ! nkey) {
    logger("strdup: %m");
    free(nvalue);
    free(nkey);
  }

  if (! (tmp = realloc(headers->elems, n * sizeof *headers->elems))) {
    logger("realloc: %m");
    goto err;
  }

  tmp[n-1].key = nkey;
  tmp[n-1].value = nvalue;
  headers->elems = tmp;
  headers->n_elems = n;

  return 0;

 err:
  return -1;
#undef HTTP_HEADER_MAX_ENTRIES
}

int http_headers_update_value(thttp_headers *headers, char *key, char *value)
{
  unsigned i = 0;
  if (! headers || ! key || ! value) {
    logger("invalid input: provide non-NULL header values");
    goto err;
  }

  for (i = 0; i < headers->n_elems; i++) {
    if (headers->elems[i].key) {
      if (0 == strcmp(key, headers->elems[i].key)) {
        char *nvalue = strdup(value);
        if (! nvalue) {
          logger("strdup: %m");
          goto err;
        }
        free(headers->elems[i].value);
        headers->elems[i].value = nvalue;
        goto found;
      }
    }
  }

 err:
  return -1;

 found:
  return (int) i;
}

int http_headers_lookup(thttp_headers *headers, char *key, char **foundp)
{
  unsigned i = 0;
  if (! headers || ! key) {
    logger("invalid input: both headers and key should be non-NULL");
    goto not_found;
  }

  for (i = 0; i < headers->n_elems; i++) {
    if (headers->elems[i].key) {
      if (0 == strcmp(key, headers->elems[i].key))
        goto found;
    }
  }

 not_found:
  return -1;

 found:
  if (foundp)
    *foundp = headers->elems[i].value;
  
  return (int) i;
}

int http_headers_foreach(thttp_headers *headers, titerate_func func, void *user_data)
{
  if (! headers)
    goto err;

  for (unsigned i = 0; i < headers->n_elems; i++) {
    struct http_headers_elem e = headers->elems[i];
    int rc = func(&e, user_data);
    if (rc < 0)
      goto err;

    // The callback did its job and asked to stop the iteration
    if (rc == 0)
      return 0;
  }

  return 0;
 err:
  return -1;

}

struct itarg {
  char *accu;
};

static int concat_headers_func(struct http_headers_elem *elem, void *user_data)
{
  struct itarg *itarg = user_data;
  char         *buf = NULL;

  if (asprintf(&buf, "%s%s: %s\r\n",
               itarg->accu ? itarg->accu : "", elem->key, elem->value) < 0) {
    logger("asprintf: %m");
    goto err;
  }

  free(itarg->accu);
  itarg->accu = buf;

  return 1;
 err:
  return -1;
}

char *http_headers_to_string(thttp_headers *headers)
{
  struct itarg itarg = { .accu = NULL };

  if (! headers)
    goto err;

  if (http_headers_foreach(headers, concat_headers_func, &itarg) < 0)
    goto err;

  return itarg.accu;
 err:
  return NULL;
}
