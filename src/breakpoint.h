/*
 * Copyright (C) 2019  T+A elektroakustik GmbH & Co. KG
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

#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#define OS_BREAKPOINT_ENABLE 0

#ifdef OS_BREAKPOINT
#undef OS_BREAKPOINT
#endif /* OS_BREAKPOINT */

#if OS_BREAKPOINT_ENABLE
#if defined __linux__ && defined __GNUC__
#if defined __i386__ || defined __amd64__
#define OS_BREAKPOINT() __asm__("int $3\n" : :)
#elif defined __arm__ || defined __aarch64__
#define OS_BREAKPOINT() __asm__("BKPT")
#endif
#endif
#endif /* !OS_BREAKPOINT_ENABLE */

#ifndef OS_BREAKPOINT
#define OS_BREAKPOINT() do {} while(0)
#endif /* !OS_BREAKPOINT */

#endif /* !BREAKPOINT_H */
