/*
 * telnetd.c - The libtelnet wrapper implementation.
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

#include <stddef.h> /* needed by libtelnet.h */
#include <stdint.h>
#include <stdlib.h>

#include <libtelnet.h>

#include "connection.h"
#include "telnetd.h"

static void telnetdEvents(telnet_t *telnet, telnet_event_t *ev, void *data)
{
  struct Connection *conn = ((struct Connection *)(data));

  if (!conn)
    return;
  switch (ev->type) {
  case TELNET_EV_DATA:
    if (connNetToHostPut(conn, (uint8_t *)(ev->data.buffer), ev->data.size) < 0)
      killConnection(conn);
    break;
  case TELNET_EV_SEND:
    if (connSend(conn, (const uint8_t *)(ev->data.buffer), ev->data.size) < 0)
      killConnection(conn);
    break;
  case TELNET_EV_DO:
    if ((ev->neg.telopt) == TELNET_TELOPT_COMPRESS2)
      telnet_begin_compress2(telnet);
    break;
  case TELNET_EV_ERROR:
    killConnection(conn);
    break;
  default:
    ;
  }
}

int telnetdInit(struct Connection *conn)
{
  const telnet_telopt_t telnetdOpts[] =
  {
    { .telopt = TELNET_TELOPT_COMPRESS2, .us = TELNET_WILL,
                                         .him = TELNET_DONT },
    { .telopt = -1, .us = 0U, .him = 0U }
  };
  char submode[2];

  if (!conn)
    return -1;
  conn->telnet = telnet_init(telnetdOpts, telnetdEvents, 0U, conn);
  telnet_negotiate(conn->telnet, TELNET_WILL, TELNET_TELOPT_COMPRESS2);
  if (!(getenv("TELNET_TELOPT_LINEMODE"))) {
    telnet_negotiate(conn->telnet, TELNET_DO, TELNET_TELOPT_LINEMODE);
    submode[0] = 1; /* MODE */
    submode[1] = 0; /* MASK */
    telnet_subnegotiation(conn->telnet, TELNET_TELOPT_LINEMODE,
                          submode, sizeof submode);
  }
  if (!(getenv("TELNET_TELOPT_ECHO")))
    telnet_negotiate(conn->telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
  return 0;
}

void telnetdStop(struct Connection *conn)
{
  if (!conn)
    return;
  if (conn->telnet)
    telnet_free(conn->telnet);
  conn->telnet = NULL;
}
