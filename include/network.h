#ifndef __NETWORK_H__
#define __NETWORK_H__

#include <stddef.h>

typedef enum {
  NETWORK_DRIVER_TYPE_PLAIN,
  NETWORK_DRIVER_TYPE_TLS,
} tnetwork_driver_type;

typedef struct network_driver_ctx tnetwork_driver_ctx;

typedef char *(* tnetwork_driver_get_name_func)(void);
typedef int (* tnetwork_driver_connect_func)(tnetwork_driver_ctx *, char *, char *, unsigned);
typedef int (* tnetwork_driver_send_func)(tnetwork_driver_ctx *, void *, size_t);
typedef int (* tnetwork_driver_recv_func)(tnetwork_driver_ctx *, unsigned char **, size_t *);
typedef void (* tnetwork_driver_free_func)(tnetwork_driver_ctx *);

void network_driver_free(tnetwork_driver_ctx *);
tnetwork_driver_ctx *network_driver_create(tnetwork_driver_type);
int network_driver_connect(tnetwork_driver_ctx *, char *, char *, unsigned);
int network_driver_send(tnetwork_driver_ctx *, void *, size_t);
int network_driver_recv(tnetwork_driver_ctx *, unsigned char **, size_t *);
tnetwork_driver_ctx *network_driver_create_by_name(char *);
tnetwork_driver_ctx *network_driver_create(tnetwork_driver_type);

struct network_driver_ctx {
  tnetwork_driver_get_name_func get_name_func;
  tnetwork_driver_connect_func  connect_func;
  tnetwork_driver_send_func     send_func;
  tnetwork_driver_recv_func     recv_func;
  tnetwork_driver_free_func     free_func;
};

#endif // __NETWORK_H__
