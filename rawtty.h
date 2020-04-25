/*
 * rawtty.h - Setup raw mode TTY interface.
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

#ifndef __RAWTTY_H
#define __RAWTTY_H

#include <termios.h>

int rawtty(int fd, struct termios *oldtermios);
void ttyreset(int fd, struct termios *oldtermios);

#endif /* __RAWTTY_H */
