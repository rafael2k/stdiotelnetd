/*
 * connection.c - TCP connection handler implementation.
 *
 * Written in 2020 by Paul Osmialowski <pawelo@king.net.pl>.
 *
 * To the extent possible under law, the author(s) have dedicated all
 * copyright and related and neighboring rights to this software to
 * the public domain worldwide. This software is distributed without
 * any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication
 * along with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0>.
 */

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ringbuf.h"

#include "connection.h"
#include "telnetd.h"

struct Connection *newConnection(const char *host, int sock)
{
  struct Connection *conn = NULL;

  assert(host);
  assert(!(sock < 0));
  conn = (struct Connection *)(malloc(sizeof(struct Connection)));
  if (!conn)
    return NULL;
  memset(conn, 0, sizeof(struct Connection));
  conn->next = NULL;
  snprintf(conn->host, sizeof conn->host, "%s", host);
  conn->sock = sock;
  conn->rbHostToNet = NULL;
  conn->rbNetToHost = NULL;
  conn->telnet = NULL;
  conn->rbHostToNet = ringbuf_new(RINGBUF_CAPACITY);
  if (!(conn->rbHostToNet)) {
    closeConnection(conn);
    return NULL;
  }
  conn->rbNetToHost = ringbuf_new(RINGBUF_CAPACITY);
  if (!(conn->rbNetToHost)) {
    closeConnection(conn);
    return NULL;
  }
  if ((telnetdInit(conn)) < 0) {
    closeConnection(conn);
    return NULL;
  }
  return conn;
}

int handleConnection(struct Connection *conn, int selected)
{
  ssize_t rec;
  size_t usize;
  uint8_t buf[RINGBUF_CAPACITY];

  assert(conn);
  if ((conn->sock) < 0)
    return -1;
  if (selected) {
    rec = recv(conn->sock, (void *)(buf), sizeof buf, MSG_NOSIGNAL);
    if (!rec)
      return -1;
    telnet_recv(conn->telnet, (char *)buf, rec);
  } else {
    usize = connHostToNetSize(conn);
    if (usize > 0) {
      if ((connHostToNetGet(conn, buf, usize)) < 0)
        return -1;
      telnet_send(conn->telnet, (char *)buf, usize);
    }
  }
  return 0;
}

int connHostToNetGet(struct Connection *conn, uint8_t *data, size_t size)
{
  assert(conn);
  assert(conn->rbHostToNet);
  if (!(ringbuf_memcpy_from(data, conn->rbHostToNet, size)))
    return -1;
  return 0;
}

int connHostToNetPut(struct Connection *conn, const uint8_t *data, size_t size)
{
  assert(conn);
  assert(conn->rbHostToNet);
  if (!(ringbuf_memcpy_into(conn->rbHostToNet, data, size)))
    return -1;
  return 0;
}

int connNetToHostGet(struct Connection *conn, uint8_t *data, size_t size)
{
  assert(conn);
  assert(conn->rbNetToHost);
  if (!(ringbuf_memcpy_from(data, conn->rbNetToHost, size)))
    return -1;
  return 0;
}

int connNetToHostPut(struct Connection *conn, const uint8_t *data, size_t size)
{
  assert(conn);
  assert(conn->rbNetToHost);
  if (!(ringbuf_memcpy_into(conn->rbNetToHost, data, size)))
    return -1;
  return 0;
}

size_t connHostToNetSize(const struct Connection *conn)
{
  assert(conn);
  assert(conn->rbHostToNet);
  return ringbuf_bytes_used(conn->rbHostToNet);
}

size_t connNetToHostSize(const struct Connection *conn)
{
  assert(conn);
  assert(conn->rbNetToHost);
  return ringbuf_bytes_used(conn->rbNetToHost);
}

int connSend(struct Connection *conn, const uint8_t *data, size_t size)
{
  ssize_t sent;

  assert(conn);
  if ((conn->sock) < 0)
    return -1;
  while (size) {
    sent = send(conn->sock, data, size, MSG_NOSIGNAL);
    if (sent == -1) {
      if (errno != EAGAIN)
        return -1;
    } else {
      if (sent > size)
        return -1;
      size -= sent;
      data += sent;
    }
  }
  return 0;
}

void killConnection(struct Connection *conn)
{
  assert(conn);
  telnetdStop(conn);
  if (conn->sock >= 0)
    close(conn->sock);
  conn->sock = -1;
}

void closeConnection(struct Connection *conn)
{
  assert(conn);
  killConnection(conn);
  if (conn->rbHostToNet)
    ringbuf_free(&(conn->rbHostToNet));
  conn->rbHostToNet = NULL;
  if (conn->rbNetToHost)
    ringbuf_free(&(conn->rbNetToHost));
  conn->rbNetToHost = NULL;
  conn->next = NULL;
  free(conn);
}
