/*
 * Copyright (C) 2016, 2019, 2023  T+A elektroakustik GmbH & Co. KG
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

#include <stdio.h>
#include <string.h>

#include "hexdump.h"

static size_t append_ascii(char *const log_buffer, size_t log_buffer_size,
                           size_t out_pos, const uint8_t *const buffer,
                           size_t start_pos, size_t end_pos,
                           size_t missing_spaces)
{
    if(missing_spaces > 0)
    {
        memset(log_buffer + out_pos, ' ', missing_spaces);
        out_pos += missing_spaces;
    }

    out_pos += snprintf(log_buffer + out_pos, log_buffer_size - out_pos, "  |");

    for(size_t i = start_pos; i < end_pos; ++i)
    {
        const char ch = buffer[i];

        if(ch >= 0x20 && ch < 0x7f)
            log_buffer[out_pos++] = ch;
        else
            log_buffer[out_pos++] = '.';
    }

    log_buffer[out_pos++] = '|';
    log_buffer[out_pos] = '\0';

    return out_pos;
}

void hexdump_to_log(enum MessageVerboseLevel level,
                    const uint8_t *const buffer, size_t buffer_length,
                    const char *what)
{
    if(!msg_is_verbose_yak(level))
        return;

    msg_vinfo(level, "%s (%zu bytes):", what, buffer_length);

    if(buffer == NULL || buffer_length == 0)
        return;

    static const size_t num_of_columns = 16;
    static const size_t sep = 8;
    char log_buffer[1024];
    size_t out_pos = 0;
    size_t start_i = 0;
    size_t i;

    for(i = 0; i < buffer_length; ++i)
    {
        if((i % num_of_columns) == 0)
            out_pos += snprintf(log_buffer + out_pos, sizeof(log_buffer) - out_pos,
                                "%04lx", (unsigned long)i);

        if((i % sep) == 0)
            log_buffer[out_pos++] = ' ';

        out_pos += snprintf(log_buffer + out_pos, sizeof(log_buffer) - out_pos, " %02x", buffer[i]);

        if((i % num_of_columns) == num_of_columns - 1)
        {
            out_pos = append_ascii(log_buffer, sizeof(log_buffer), out_pos,
                                   buffer, start_i, i + 1, 0);
            msg_vinfo(level, "%s", log_buffer);
            out_pos = 0;
            start_i = i + 1;
        }
    }

    if(out_pos > 0)
    {
        const size_t missing_columns =
            num_of_columns - (buffer_length - start_i);

        append_ascii(log_buffer, sizeof(log_buffer), out_pos,
                     buffer, start_i, buffer_length,
                     3 * missing_columns + missing_columns / sep);
        msg_vinfo(level, "%s", log_buffer);
    }
}
