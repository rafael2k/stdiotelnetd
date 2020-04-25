/*
 * spawn.c - Spawn process and redirect stdin/stdout implementation.
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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>

#include "spawn.h"

#define READ 0
#define WRITE 1

pid_t spawn(const char *path, int argc, char **argv, int *fdin, int *fdout)
{
  int p_stdin[] = { -1, -1 };
  int p_stdout[] = { -1, -1 };
  pid_t pid;
  int i;

  if (argc > 1) {
    if ((argc < 3) || (strcmp("--", argv[1])))
      return -1;
  }
  if (pipe(p_stdin))
    return -1;
  if (pipe(p_stdout))
  {
    if (!(p_stdin[READ] < 0)) close(p_stdin[READ]);
    if (!(p_stdin[WRITE] < 0)) close(p_stdin[WRITE]);
    return -1;
  }
  assert(!(p_stdin[READ] < 0));
  assert(!(p_stdin[WRITE] < 0));
  assert(!(p_stdout[READ] < 0));
  assert(!(p_stdout[WRITE] < 0));
  pid = fork();
  if (pid < 0)
    return pid;
  if (!pid)
  {
    signal(SIGCHLD, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
    if (setpgrp())
      _Exit(-1);
    close(p_stdin[WRITE]);
    if (dup2(p_stdin[READ], READ) < 0)
      _Exit(-1);
    close(p_stdout[READ]);
    if (dup2(p_stdout[WRITE], WRITE) < 0)
      _Exit(-1);
    if (argc > 1) {
      for (i = 1; i < (argc - 1); i++)
        argv[i] = argv[i + 1];
      argv[argc - 1] = NULL;
      execvp(argv[0], argv);
      perror("execvp");
    } else {
      execlp(argv[0], argv[0], NULL);
      perror("execlp");
    }
    _Exit(-1);
  } else {
    if (!fdin)
      close(p_stdin[WRITE]);
    else
      *fdin = p_stdin[WRITE];
    if (!fdout)
      close(p_stdout[READ]);
    else
      *fdout = p_stdout[READ];
  }
  return pid;
}
