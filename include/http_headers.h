#ifndef __HTTP_HEADERS_H__
#define __HTTP_HEADERS_H__

struct http_headers_elem {
  char *key;
  char *value;
};
typedef struct http_headers thttp_headers;
typedef int (*titerate_func)(struct http_headers_elem *, void *);

void http_headers_free(thttp_headers *headers);
int http_headers_new(char *key, char *value, thttp_headers **headersp);
int http_headers_add(thttp_headers *headers, char *key, char *value);
int http_headers_update_value(thttp_headers *headers, char *key, char *value);
int http_headers_lookup(thttp_headers *headers, char * key, char **valuep);
int http_headers_foreach(thttp_headers *headers, titerate_func func, void *user_data);
char *http_headers_to_string(thttp_headers *headers);

#endif // __HTTP_HEADERS_H__
