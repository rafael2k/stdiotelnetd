/*
 * server.c - General purpose TCP server implementation.
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

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/select.h>

#include "ringbuf.h"

#include "server.h"
#include "connection.h"

#define CONNMAXNUMBER 10

int serverInit(struct Server *server, uint16_t waitport)
{
  int sock = -1;
  int on = 1;
  struct sockaddr_in sa_server;

  assert(server);
  assert(waitport > 0U);
  memset(server, 0, sizeof(struct Server));
  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock < 0)
    return -1;
  memset(&sa_server, 0, sizeof sa_server);
  sa_server.sin_family = AF_INET;
  sa_server.sin_addr.s_addr = INADDR_ANY;
  sa_server.sin_port = htons(waitport);
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on) < 0) {
    close(sock);
    return -1;
  }
  if (bind(sock, ((struct sockaddr *)(&sa_server)), sizeof sa_server) < 0) {
    close(sock);
    return -1;
  }
  if (listen(sock, CONNMAXNUMBER) < 0) {
    close(sock);
    return -1;
  }
  server->waitsock = sock;
  server->connections = NULL;
  server->rbHostToNet = NULL;
  server->rbNetToHost = NULL;
  server->rbHostToNet = ringbuf_new(RINGBUF_CAPACITY);
  if (!(server->rbHostToNet)) {
    serverStop(server);
    return -1;
  }
  server->rbNetToHost = ringbuf_new(RINGBUF_CAPACITY);
  if (!(server->rbNetToHost)) {
    serverStop(server);
    return -1;
  }
  return 0;
}

int serverStep(struct Server *server)
{
  struct Connection *conn = NULL;
  struct Connection *prev = NULL;
  const char *motd = getenv("TELNET_MOTD");
  fd_set fdset;
  struct timeval tv;
  int max = -1;
  int selected = 0;
  struct sockaddr_in sa_client;
  socklen_t addrlen = sizeof sa_client;
  int sock = -1;
  char topbuf[512U];
  uint8_t outbuf[RINGBUF_CAPACITY];
  size_t outsize;
  size_t insize;

  assert(server);
  assert(!((server->waitsock) < 0));
  conn = server->connections;
  outsize = serverHostToNetSize(server);
  assert(outsize < sizeof outbuf);
  if (outsize > 0U) {
    if (serverHostToNetGet(server, outbuf, outsize) < 0)
      return -1;
  }
  FD_ZERO(&fdset);
  FD_SET(server->waitsock, &fdset);
  max = server->waitsock;
  while (conn) {
    if (conn->sock >= 0) {
      FD_SET(conn->sock, &fdset);
      if (conn->sock > max) max = conn->sock;
    }
    conn = conn->next;
  }
  assert(!(max < 0));
  tv.tv_sec = 0;
  tv.tv_usec = 200;
  selected = select(max + 1, &fdset, NULL, NULL, &tv);
  conn = server->connections;
  if ((selected > 0) && (FD_ISSET(server->waitsock, &fdset))) {
    sock = accept(server->waitsock, ((struct sockaddr *)(&sa_client)),
                  &addrlen);
    if (!(sock < 0)) {
      prev = server->connections;
      server->connections = newConnection(inet_ntop(AF_INET,
                                                    &sa_client.sin_addr,
                                                    topbuf,
                                                    sizeof topbuf),
                                          sock);
      if ((server->connections) && motd) {
        if ((connSendMsg(server->connections, motd)) < 0) {
          closeConnection(server->connections);
          server->connections = NULL;
        } else if ((connSendMsg(server->connections, "\n\r")) < 0) {
          closeConnection(server->connections);
          server->connections = NULL;
        }
      }
      if (server->connections) {
        assert(server->connections->sock == sock);
        server->connections->next = prev;
      } else {
        server->connections = prev;
        close(sock);
        sock = -1;
      }
    }
  }
  prev = NULL;
  while (conn) {
    if (outsize > 0U) {
      if (connHostToNetPut(conn, outbuf, outsize) < 0) {
        closeConnection(conn);
        return -1;
      }
    }
    if (handleConnection(conn,
                         ((selected > 0) && FD_ISSET(conn->sock, &fdset)))) {
      if (prev) {
        prev->next = conn->next;
        closeConnection(conn);
        conn = prev->next;
      } else {
        server->connections = conn->next;
        closeConnection(conn);
        conn = server->connections;
      }
    } else {
      insize = connNetToHostSize(conn);
      if (insize > 0U) {
        if (!(ringbuf_copy(server->rbNetToHost, conn->rbNetToHost ,insize)))
          closeConnection(conn);
      }
      prev = conn;
      conn = conn->next;
    }
  }
  return 0;
}

void serverStop(struct Server *server)
{
  struct Connection *conn = NULL;
  struct Connection *tmp = NULL;

  assert(server);
  conn = server->connections;
  while (conn) {
    tmp = conn;
    conn = conn->next;
    closeConnection(tmp);
    tmp = NULL;
  }
  if (server->rbHostToNet)
    ringbuf_free(&(server->rbHostToNet));
  server->rbHostToNet = NULL;
  if (server->rbNetToHost)
    ringbuf_free(&(server->rbNetToHost));
  server->rbNetToHost = NULL;
  if ((server->waitsock) >= 0)
    close(server->waitsock);
  server->waitsock = -1;
}

int serverHostToNetGet(struct Server *server, uint8_t *data, size_t size)
{
  assert(server);
  assert(server->rbHostToNet);
  if (!(ringbuf_memcpy_from(data, server->rbHostToNet, size)))
    return -1;
  return 0;
}

int serverHostToNetPut(struct Server *server, const uint8_t *data, size_t size)
{
  assert(server);
  assert(server->rbHostToNet);
  if (!(ringbuf_memcpy_into(server->rbHostToNet, data, size)))
    return -1;
  return 0;
}

int serverNetToHostGet(struct Server *server, uint8_t *data, size_t size)
{
  assert(server);
  assert(server->rbNetToHost);
  if (!(ringbuf_memcpy_from(data, server->rbNetToHost, size)))
    return -1;
  return 0;
}

int serverNetToHostPut(struct Server *server, const uint8_t *data,
                       size_t size)
{
  assert(server);
  assert(server->rbNetToHost);
  if (!(ringbuf_memcpy_into(server->rbNetToHost, data, size)))
    return -1;
  return 0;
}

size_t serverHostToNetSize(const struct Server *server)
{
  assert(server);
  assert(server->rbHostToNet);
  return ringbuf_bytes_used(server->rbHostToNet);
}

size_t serverNetToHostSize(const struct Server *server)
{
  assert(server);
  assert(server->rbNetToHost);
  return ringbuf_bytes_used(server->rbNetToHost);
}
