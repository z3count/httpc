#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>

#include "logger.h"
#include "network_plain.h"
#include "network.h"

typedef struct {
  tnetwork_driver_ctx driver; // Mandatory as first element of the struct

  int fd;

} tnetwork_driver_plain_ctx;

static int set_nonblock(int fd, int nonblock)
{
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0)
    return -1;

  return fcntl(fd, F_SETFL, nonblock ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
}

static int set_timeouts(int fd, unsigned timeout_sec)
{
  struct timeval tv = {
    .tv_sec = timeout_sec,
    .tv_usec = 0
  };

  if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv) < 0) {
    logger("setsockopt: %m");
    return -1;
  }

  if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) < 0) {
    logger("setsockopt: %m");
    return -1;
  }

  return 0;
}

static int wait_fd(int fd, int do_write, unsigned timeout_sec)
{ 
  struct timeval tv = {
    .tv_sec = timeout_sec,
    .tv_usec = 0
  };
  fd_set rfds, wfds;

  FD_ZERO(&rfds);
  FD_ZERO(&wfds);

  if (do_write)
    FD_SET(fd, &wfds);
  else
    FD_SET(fd, &rfds);

  if (select(fd + 1, do_write ? NULL : &rfds, do_write ? &wfds : NULL, NULL, &tv) <= 0) {
    logger("select: %m");
    return -1;
  }

  return 0;
}

static int network_driver_plain_connect(tnetwork_driver_ctx *ctx, char *host, char *port, unsigned timeout_sec)
{
  tnetwork_driver_plain_ctx *driver_ctx = (tnetwork_driver_plain_ctx *) ctx;

  struct addrinfo hints, *res = NULL, *rp = NULL;
  int fd = -1, gai = -1;

  memset(&hints, 0, sizeof hints);
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if ((gai = getaddrinfo(host, port, &hints, &res))) {
    logger("getaddrinfo: %m");
    return -1;
  }

  for (rp = res; rp; rp = rp->ai_next) {
    if ((fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) < 0)
      continue;

    if (set_nonblock(fd, 1) < 0) {
      logger("fcntl: %m");
      (void) close(fd);
      fd = -1;
      continue;
    }

    if (connect(fd, rp->ai_addr, rp->ai_addrlen) < 0 && errno != EINPROGRESS) {
      logger("connect: %m");
      (void) close(fd);
      fd = -1;
      continue;
    }

    if (! wait_fd(fd, 1, timeout_sec)) {
      int err = 0; socklen_t len = sizeof err;
      if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len) == 0 && err == 0) {
        if (set_nonblock(fd, 0) < 0) {
          logger("fcntl: %m");
          (void) close(fd);
          fd = -1;
          continue;
        }

        if (set_timeouts(fd, timeout_sec) < 0) {
          logger("setsockopt: %m");
          (void) close(fd);
          fd = -1;
          continue;
        }
        break;
      } else {
        logger("connect: %m");
      }
    } else {
      logger("connect: timeout after %d sec\n", timeout_sec);
    }

    (void) close(fd);
    fd = -1;
  }

  driver_ctx->fd = fd;
  freeaddrinfo(res);

  return fd;
}

static int network_driver_plain_send(tnetwork_driver_ctx *ctx, void *buf, size_t len)
{
  tnetwork_driver_plain_ctx *driver_ctx = (tnetwork_driver_plain_ctx *) ctx;
  unsigned char             *p = (unsigned char *) buf;
  size_t                    off = 0;

  while (off < len) {
    ssize_t n = send(driver_ctx->fd, p + off, len - off, 0);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return -1;
    }
    off += (size_t) n;
  }

  return 0;
}    

static int network_driver_plain_recv(tnetwork_driver_ctx *ctx, unsigned char **datap, size_t *data_lenp)
{
#define DEFAULT_RECV_BUF_SIZE 4096
#define MAX_RECV_BUF_SIZE (128 * 1024)

  tnetwork_driver_plain_ctx *driver_ctx = (tnetwork_driver_plain_ctx *) ctx;
  size_t                     n_allocated = 0;
  size_t                     data_len = 0;
  unsigned char             *buf = NULL;

  n_allocated =  DEFAULT_RECV_BUF_SIZE * sizeof *buf;
  if (NULL == (buf = calloc(1, n_allocated))) {
    logger("calloc: %m");
    goto err;
  }
  
  while (1) {
    ssize_t n = recv(driver_ctx->fd, buf + data_len, n_allocated - data_len, 0);
    if (0 == n) {
      // Got the EOF
      goto end;
    } else if (n > 0) {
      data_len += (size_t) n;

      // We might have to grow the buffer
      if (data_len >= n_allocated) {
        n_allocated *= 2;
        if (n_allocated > MAX_RECV_BUF_SIZE) {
          logger("Can't grow the receiving buffer above %dKB\n", MAX_RECV_BUF_SIZE/1024);
          goto err;
        }

        // Exponential growth, it's suboptimal but hey.
        void *tmpbuf = realloc(buf, n_allocated);
        if (! tmpbuf) {
          logger("realloc: %m");
          goto err;
        }

        buf = tmpbuf;
      }
      
      continue;
    } else {
      if (n < 0) {
        if (errno == EINTR)
          continue;

        goto err;
      }
    }

    break;
  }

 end:
  *datap = buf;
  buf[data_len] = 0;
  *data_lenp = data_len;
  return 0;

 err:
  free(buf);
  return -1;
}

static char *network_driver_plain_get_name(void)
{
  return "plain";
}

static void network_driver_plain_free(tnetwork_driver_ctx *ctx)
{
  tnetwork_driver_plain_ctx *driver_ctx = (tnetwork_driver_plain_ctx *) ctx;

  if (driver_ctx->fd >= 0)
    (void) close(driver_ctx->fd);

  free(driver_ctx);
}

tnetwork_driver_ctx *network_driver_plain_create(void)
{
  tnetwork_driver_plain_ctx *ctx = calloc(1, sizeof *ctx);
  if (! ctx) {
    logger("calloc: %m");
    return NULL;
  }

  ctx->fd = -1;

  ctx->driver.connect_func  = network_driver_plain_connect;
  ctx->driver.send_func     = network_driver_plain_send;
  ctx->driver.recv_func     = network_driver_plain_recv;
  ctx->driver.get_name_func = network_driver_plain_get_name;
  ctx->driver.free_func     = network_driver_plain_free;

  return (tnetwork_driver_ctx *) ctx;
}
