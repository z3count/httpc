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

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#include "logger.h"
#include "network.h"
#include "network_tls.h"


typedef struct {
  tnetwork_driver_ctx driver; // Mandatory as first element of the struct

  SSL_CTX *ssl_ctx;
  SSL     *ssl;
  
  int fd;

  char *port;
  char *host;
} tnetwork_driver_tls_ctx;

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

static int network_driver_tls_connect(tnetwork_driver_ctx *ctx, char *host, char *port, unsigned timeout_sec)
{
  tnetwork_driver_tls_ctx *driver_ctx = (tnetwork_driver_tls_ctx *) ctx;
  struct addrinfo hints, *res = NULL, *rp = NULL;
  int fd = -1, gai = -1;

  memset(&hints, 0, sizeof hints);
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  if (! (driver_ctx->host = strdup(host))) {
    logger("strdup: %m");
    return -1;
  }
  
  if ((gai = getaddrinfo(driver_ctx->host, port, &hints, &res))) {
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

static int network_driver_tls_send(tnetwork_driver_ctx *ctx, void *buf, size_t buf_size)
{
  tnetwork_driver_tls_ctx *driver_ctx = (tnetwork_driver_tls_ctx *) ctx;
  int                      ret = -1;
  X509_VERIFY_PARAM       *param = NULL;

  driver_ctx->ssl_ctx = SSL_CTX_new(TLS_client_method());
  if (! driver_ctx->ssl_ctx) {
    logger("SSL_CTX_new failed");
    goto end;
  }

  // Use the trusted paths for CA
  if (SSL_CTX_set_default_verify_paths(driver_ctx->ssl_ctx) != 1) {
    logger("SSL_CTX_set_default_verify_paths failed");
    goto end;
  }

  if (! (driver_ctx->ssl = SSL_new(driver_ctx->ssl_ctx))) {
    logger("SSL_new failed");
    goto end;
  }

  // fd to ssl context association
  if (SSL_set_fd(driver_ctx->ssl, driver_ctx->fd) != 1) {
    logger("SSL_set_fd failed");
    goto end;
  }

  // Set the SNI
  if (SSL_set_tlsext_host_name(driver_ctx->ssl, driver_ctx->host) != 1) {
    logger("SNI failed");
    goto end;
  }

  // Ask for certificate verification
  SSL_set_verify(driver_ctx->ssl, SSL_VERIFY_PEER, NULL);

  param = SSL_get0_param(driver_ctx->ssl);

  // Make sure the hostname matches
  X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
  if (! X509_VERIFY_PARAM_set1_host(param, driver_ctx->host, 0)) {
    logger("X509_VERIFY_PARAM_set1_host failed");
    goto end;
  }

  if (SSL_connect(driver_ctx->ssl) != 1) {
    logger("SSL_connect failed");
    ERR_print_errors_fp(stderr);
    goto end;
  }

  // Result chain verification
  long verr = SSL_get_verify_result(driver_ctx->ssl);
  if (verr != X509_V_OK) {
    logger("Cert verify failed: %ld (%s)", verr, X509_verify_cert_error_string(verr));
    goto end;
  }

  if (SSL_write(driver_ctx->ssl, buf, buf_size) < 0) {
    logger("SSL_write failed");
    ERR_print_errors_fp(stderr);
    goto end;
  }

  ret = 0;
 end:
  return ret;
}    

static int network_driver_tls_recv(tnetwork_driver_ctx *ctx, unsigned char **datap, size_t *data_lenp)
{
#define DEFAULT_RECV_BUF_SIZE 4096
#define MAX_RECV_BUF_SIZE (128 * 1024)

  tnetwork_driver_tls_ctx *driver_ctx = (tnetwork_driver_tls_ctx *) ctx;
  size_t                   n_allocated = 0;
  size_t                   data_len = 0;
  unsigned char           *buf = NULL;

  n_allocated =  DEFAULT_RECV_BUF_SIZE * sizeof *buf;
  if (NULL == (buf = calloc(1, n_allocated))) {
    logger("calloc: %m");
    goto err;
  }
  
  while (1) {
    int n = SSL_read(driver_ctx->ssl, buf + data_len, n_allocated - data_len);
    if (0 == n) {
      // Got the EOF
      goto end;
    } else if (n > 0) {
      // Success!  we actually got something to read.
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
      
      goto again;
    } else {
      // Failure or clean EOF?  Let's check first for any error.

      switch(SSL_get_error(driver_ctx->ssl, n)) {
      case SSL_ERROR_ZERO_RETURN:
        goto end;

      case SSL_ERROR_WANT_READ:
      case SSL_ERROR_WANT_WRITE:
        // Retry!
        goto again;

      case 0:
        // Closed socket.
        goto end;

      default:
        logger("SSL_read failed");
        ERR_print_errors_fp(stderr);
        goto end;
      }
    }

  again:
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

static char *network_driver_tls_get_name(void)
{
  return "tls";
}

static void network_driver_tls_free(tnetwork_driver_ctx *ctx)
{
  tnetwork_driver_tls_ctx *driver_ctx = (tnetwork_driver_tls_ctx *) ctx;

  if (driver_ctx->ssl) {
    // Envoi d’un close_notify (best effort)
    SSL_shutdown(driver_ctx->ssl);
    SSL_free(driver_ctx->ssl);
  }

  if (driver_ctx->ssl_ctx)
    SSL_CTX_free(driver_ctx->ssl_ctx);

  free(driver_ctx->host);

  if (driver_ctx->fd >= 0)
    (void) close(driver_ctx->fd);

  free(driver_ctx);
}

tnetwork_driver_ctx *network_driver_tls_create(void)
{
  tnetwork_driver_tls_ctx *ctx = calloc(1, sizeof *ctx);
  if (! ctx) {
    logger("calloc: %m");
    return NULL;
  }

  ctx->fd = -1;

  ctx->driver.connect_func  = network_driver_tls_connect;
  ctx->driver.send_func     = network_driver_tls_send;
  ctx->driver.recv_func     = network_driver_tls_recv;
  ctx->driver.get_name_func = network_driver_tls_get_name;
  ctx->driver.free_func     = network_driver_tls_free;

  return (tnetwork_driver_ctx *) ctx;
}
