/*
 * debug.h - A macro for controlling debug output.
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


#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef DEBUG
#include <stdio.h>

#define D(...) fprintf(stderr, __VA_ARGS__)
#else
#define D(...)
#endif

#endif /* __DEBUG_H */
