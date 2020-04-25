/*
 * telnetd.h - The libtelnet wrapper interface.
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

#ifndef __TELNETD_H
#define __TELNETD_H

#include "connection.h"

int telnetdInit(struct Connection *conn);
void telnetdStop(struct Connection *conn);

#endif /* __TELNETD_H */
