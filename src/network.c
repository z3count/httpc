#include <string.h>

#include "util.h"
#include "logger.h"
#include "network.h"
#include "network_plain.h"
#include "network_tls.h"

static struct driver_mapping {
  tnetwork_driver_type type;
  char *name;
  tnetwork_driver_ctx *(*cb_create)(void);
} driver_mapping[] = {
  {
    NETWORK_DRIVER_TYPE_PLAIN,
    "plain",
    network_driver_plain_create
  },
  {
    NETWORK_DRIVER_TYPE_TLS,
    "tls",
    network_driver_tls_create
  }
};

char *network_driver_type_to_str(tnetwork_driver_type type)
{
  switch (type) {
#define MAP(x) case NETWORK_DRIVER_TYPE_##x : return #x
    MAP(PLAIN);
    MAP(TLS);
#undef MAP
  }

  return "<unknown type>";
}

char *network_driver_get_name(tnetwork_driver_ctx *ctx)
{
  return ctx->get_name_func();
}

int network_driver_connect(tnetwork_driver_ctx *ctx, char *host, char *port, unsigned timeout_sec)
{
  return ctx->connect_func(ctx, host, port, timeout_sec);
}

int network_driver_send(tnetwork_driver_ctx *ctx, void *buf, size_t buf_size)
{
  return ctx->send_func(ctx, buf, buf_size);
}

int network_driver_recv(tnetwork_driver_ctx *ctx, unsigned char **bufp, size_t *buf_sizep)
{
  return ctx->recv_func(ctx, bufp, buf_sizep);
}

void network_driver_free(tnetwork_driver_ctx *ctx)
{
  if (! ctx)
    return;

  ctx->free_func(ctx);
}

static int cmp_name_cb(void *map_, void *name_)
{
  struct driver_mapping *map = map_;
  char *name = name_;

  return 0 == strcmp(name, map->name);
}

static char *name_str_cb(void *name)
{
  return (char *) name;
}

static int cmp_type_cb(void *map_, void *type_)
{
  struct driver_mapping *map = map_;
  tnetwork_driver_type *type = type_;

  return *type == map->type;
}

static char *type_str_cb(void *type)
{
  return network_driver_type_to_str(*(tnetwork_driver_type *) type);
}

static tnetwork_driver_ctx *network_driver_create_internal(void *data,
                                                           int (*cb)(void *, void *),
                                                           char *(*to_str_cb)(void *))
{
  tnetwork_driver_ctx *ctx = NULL;

  for (size_t i = 0; i < N_ELEMS(driver_mapping); i++) {
    if (cb(data, &driver_mapping[i])) {
      ctx = driver_mapping[i].cb_create();
      if (! ctx) {
        logger("failed to create a network driver "
               "for type \"%s\"", to_str_cb(data));
        goto end;
      }

      break;
    }
  }

  /* No driver matching the provided type was found */
  if (! ctx)
    logger("Unknown network driver for type \"%s\"",
           to_str_cb(data));
 end:
  return ctx;
}

tnetwork_driver_ctx *network_driver_create_by_name(char *name)
{
  return network_driver_create_internal(name, cmp_name_cb, name_str_cb);
}

tnetwork_driver_ctx *network_driver_create(tnetwork_driver_type type)
{
  return network_driver_create_internal(&type, cmp_type_cb, type_str_cb);
}
