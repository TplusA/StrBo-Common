/*
 * Copyright (C) 2020  T+A elektroakustik GmbH & Co. KG
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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include "backtrace.h"
#include "messages.h"

#include <execinfo.h>
#include <stdio.h>

static inline void **gather_backtrace(size_t depth, size_t *size)
{
#ifdef __cplusplus
    thread_local
#else /* !__cplusplus */
    _Thread_local
#endif /* __cplusplus */
    static void *buffer[100];

    depth = depth > 0 && depth <= sizeof(buffer) / sizeof(buffer[0])
        ? depth
        : sizeof(buffer) / sizeof(buffer[0]);

    *size = backtrace(buffer, depth);
    return buffer;
}

void backtrace_dump(size_t depth, const char *message)
{
    size_t size;
    void **buffer = gather_backtrace(depth, &size);

    if(message != NULL)
        fprintf(stderr, "--- Stack trace -- %s ---\n", message);
    else
        fprintf(stderr, "--- Stack trace ---\n");

    if(buffer == NULL)
        fprintf(stderr, "<OUT OF MEMORY>");
    else
        backtrace_symbols_fd(buffer, size, STDERR_FILENO);

    fprintf(stderr, "-----------------\n");
}

void backtrace_log(size_t depth, const char *message)
{
    size_t size;
    void **buffer = gather_backtrace(depth, &size);

    if(message != NULL)
        msg_error(0, LOG_WARNING, "--- Stack trace -- %s ---", message);
    else
        msg_error(0, LOG_WARNING, "--- Stack trace ---");

    char **symbols = backtrace_symbols(buffer, size);

    if(symbols == NULL)
        msg_out_of_memory("backtrace");
    else
    {
        for(size_t i = 0; i < size; ++i)
            msg_error(0, LOG_WARNING, "%s", symbols[i]);

        free(symbols);
    }

    msg_error(0, LOG_WARNING, "-----------------");
}

