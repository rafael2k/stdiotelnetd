/*
 * main.c - A simple TCP server using telnet protocol for communication.
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
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "debug.h"
#include "server.h"
#include "rawtty.h"
#include "spawn.h"

#define FAIL -1

static volatile int quit = 0;

static void sigHandler(int sig)
{
  sig = sig;
  quit = !0;
}

int main(int argc, char **argv)
{
  int isRaw = 0;
  pid_t spawned = 0;
  int fdin = fileno(stdin);
  int fdout = fileno(stdout);
  uint16_t waitport;
  struct Server server;
  struct termios oldtermios;
  fd_set fds;
  struct timeval tv;
  size_t sigDone;
  size_t usize;
  ssize_t ssize;
  uint8_t buf[RINGBUF_CAPACITY];
  const uint8_t *bufptr;
  int retval;

  memset(&server, 0, sizeof server);
  memset(&oldtermios, 0, sizeof oldtermios);
  if (argc == 1) {
    fprintf(stderr, "Usage: %s <waitport> [<cmd> [-- [<args>]]]\n", argv[0]);
    return FAIL;
  }
  if (!(argc > 1))
    return FAIL;
  waitport = atoi(argv[1]);
  if (!(waitport > 0)) {
    fprintf(stderr, "Invalid wait port.\n");
    return FAIL;
  }
  D("Starting %s on port %u.\n", argv[0], waitport);
  if (serverInit(&server, waitport)) {
    fprintf(stderr, "Cannot start server.\n");
    return FAIL;
  }
  if (argc > 2) {
    spawned = spawn(argv[2], argc - 2, argv + 2, &fdout, &fdin);
    if (spawned < 0) {
      fprintf(stderr, "Could not execute your command.\n");
      serverStop(&server);
      return FAIL;
    }
  }
  sigDone = 0U;
  do {
    if (SIG_ERR == signal(SIGPIPE, sigHandler))
      break;
    sigDone++;
    if (SIG_ERR == signal(SIGTERM, sigHandler))
      break;
    sigDone++;
    if (SIG_ERR == signal(SIGQUIT, sigHandler))
      break;
    sigDone++;
    if (SIG_ERR == signal(SIGINT, sigHandler))
      break;
    sigDone++;
    if (SIG_ERR == signal(SIGHUP, sigHandler))
      break;
    sigDone++;
    if (SIG_ERR == signal(SIGCHLD, sigHandler))
      break;
    sigDone++;
  } while (0);
  if (sigDone < 6U) {
    fprintf(stderr, "Cannot arm signals.\n");
    if (spawned)
      kill(spawned, SIGKILL);
    serverStop(&server);
    return FAIL;
  }
  if ((!spawned) && (!(getenv("TELNET_TELOPT_LINEMODE")))) {
    isRaw = !((rawtty(fdin, &oldtermios)) < 0);
    if (!isRaw)
      fprintf(stderr, "Cannot set raw tty.\n");
  }
  retval = 0;
  while (!quit) {
    FD_ZERO(&fds);
    FD_SET(fdin, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 10;
    if (select(fdin + 1, &fds, NULL, NULL, &tv)) {
      if ((!quit) && (FD_ISSET(fdin, &fds))) {
        buf[0] = 0U;
        ssize = read(fdin, buf, sizeof buf);
        if (ssize > 0) {
          if ((serverHostToNetPut(&server, buf, ssize)) < 0) {
            fprintf(stderr, "Ringbuf failure (OUT).\n");
            retval = FAIL;
            break;
          }
        } else {
          break;
        }
      }
    }
    usize = serverNetToHostSize(&server);
    if (usize > sizeof buf) {
      fprintf(stderr, "Internal error: buffer too small.\n");
      retval = FAIL;
      break;
    }
    if (usize > 0) {
      buf[0] = 0U;
      if ((serverNetToHostGet(&server, buf, usize)) < 0) {
        fprintf(stderr, "Ringbuf failure (IN).\n");
        retval = FAIL;
        break;
      }
      bufptr = buf;
      while (usize) {
        ssize = write(fdout, bufptr, usize);
        if (ssize < 0) {
          if (errno != EAGAIN)
            break;
        }
        if (ssize > usize)
          break;
        usize -= ssize;
        bufptr += ssize;
      }
      if (usize) {
        fprintf(stderr, "Write error.\n");
        retval = FAIL;
        break;
      }
    }
    if (serverStep(&server)) {
      fprintf(stderr, "Emergency exit.\n");
      retval = FAIL;
      break;
    }
  }
  if (spawned)
    kill(spawned, retval ? SIGKILL : SIGINT);
  if (isRaw)
    ttyreset(fdin, &oldtermios);
  serverStop(&server);
  D("\r\nNatural end.\r\n");
  return retval;
}
