/*
 * connection.h - TCP connection handler interface.
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

#ifndef __CONNECTION_H
#define __CONNECTION_H

#include <stddef.h> /* needed by libtelnet.h */
#include <stdint.h>
#include <libtelnet.h>

#include <sys/socket.h> /* MSG_NOSIGNAL */

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#include "ringbuf.h"

#define MAX_HOST_LEN 127U

struct Connection
{
  struct Connection *next;
  char host[MAX_HOST_LEN + 1U];
  int sock;
  ringbuf_t rbHostToNet;
  ringbuf_t rbNetToHost;
  telnet_t *telnet;
};

struct Connection *newConnection(const char *host, int sock);
int handleConnection(struct Connection *conn, int selected);
int connSend(struct Connection *conn, const uint8_t *data, size_t size);
int connSendMsg(struct Connection *conn, const char *msg);
int connHostToNetGet(struct Connection *conn, uint8_t *data, size_t size);
int connHostToNetPut(struct Connection *conn, const uint8_t *data, size_t size);
int connNetToHostGet(struct Connection *conn, uint8_t *data, size_t size);
int connNetToHostPut(struct Connection *conn, const uint8_t *data, size_t size);
size_t connHostToNetSize(const struct Connection *conn);
size_t connNetToHostSize(const struct Connection *conn);
void killConnection(struct Connection *conn);
void closeConnection(struct Connection *conn);

#endif /* __CONNECTION_H */
