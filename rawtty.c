/*
 * rawtty.c - Setup raw mode TTY implementation.
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

#include <termios.h>
#include <unistd.h>

int rawtty(int fd, struct termios *oldtermios)
{
  struct termios newtermios;

  if ((fd < 0) || (!oldtermios))
    return -1;
  if (tcgetattr(fd, oldtermios))
    return -1;
  newtermios = *oldtermios;
  newtermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  newtermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  newtermios.c_cflag &= ~(CSIZE | PARENB);
  newtermios.c_cflag |= CS8;
  newtermios.c_oflag &= ~(OPOST);
  newtermios.c_cc[VMIN] = 1;
  newtermios.c_cc[VTIME] = 0;
  return tcsetattr(fd, TCSAFLUSH, &newtermios);
}

void ttyreset(int fd, struct termios *oldtermios)
{
  if (!(fd < 0))
    tcsetattr(fd, TCSAFLUSH, oldtermios);
}
