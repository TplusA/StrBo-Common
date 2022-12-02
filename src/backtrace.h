/*
 * Copyright (C) 2020, 2022  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of the T+A Streaming Board software stack ("StrBoWare").
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#ifndef BACKTRACE_H
#define BACKTRACE_H

#include <stdlib.h>

#ifdef __GNUC__
#ifdef __cplusplus
extern "C" {
#endif
/*
 * --- NOTE --- NOTE --- NOTE --- NOTE --- NOTE --- NOTE --- NOTE --- NOTE ---
 *
 * To make this work well with gcc, you need to
 *   - pass -g and -funwind-tables to the compiler, and
 *   - pass -g and -rdynamic to the linker.
 *
 * It is also a good idea to disable compiler optimizations (-Og or -O0) as
 * well as linker optimizations (-Wl,-O0) to enable more meaningful traces.
 *
 * --- NOTE --- NOTE --- NOTE --- NOTE --- NOTE --- NOTE --- NOTE --- NOTE ---
 */

/*!
 * Dump stack trace to \c stderr.
 *
 * Use this function in unit tests where necessary to avoid messing with log
 * message expectations.
 *
 * Do not keep any calls of this function in production code!
 *
 * \param depth
 *     Maximum number of lines in the stack trace. Pass 0 to dump as many lines
 *     as possible.
 *
 * \param message
 *     Stack trace header.
 */
void backtrace_dump(size_t depth, const char *message);

/*!
 * Dump stack trace to system log.
 *
 * Do not keep any calls of this function in production code!
 *
 * \param depth
 *     Maximum number of lines in the stack trace. Pass 0 to dump as many lines
 *     as possible.
 *
 * \param message
 *     Stack trace header.
 */
void backtrace_log(size_t depth, const char *message);
#ifdef __cplusplus
}
#endif
#else /* !__GNUC__ */
static inline void backtrace_dump(size_t depth, const char *message) {}
static inline void backtrace_log(size_t depth, const char *message) {}
#endif /* __GNUC__ */

#endif /* !BACKTRACE_H */
