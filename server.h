/*
 * server.h - General purpose TCP server interface.
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

#ifndef __SERVER_H
#define __SERVER_H

#include <stddef.h>
#include <stdint.h>

#include "ringbuf.h"

#include "connection.h"

struct Server
{
  struct Connection *connections;
  int waitsock;
  ringbuf_t rbHostToNet;
  ringbuf_t rbNetToHost;
};

int serverInit(struct Server *server, uint16_t waitport);
int serverStep(struct Server *server);
void serverStop(struct Server *server);
int serverHostToNetGet(struct Server *server, uint8_t *data, size_t size);
int serverHostToNetPut(struct Server *server, const uint8_t *data, size_t size);
int serverNetToHostGet(struct Server *server, uint8_t *data, size_t size);
int serverNetToHostPut(struct Server *server, const uint8_t *data, size_t size);
size_t serverHostToNetSize(const struct Server *server);
size_t serverNetToHostSize(const struct Server *server);

#endif /* __SERVER_H */
