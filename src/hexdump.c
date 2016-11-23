/*
 * Copyright (C) 2016  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of the T+A Streaming Board software stack ("StrBoWare").
 *
 * StrBoWare is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3 as
 * published by the Free Software Foundation.
 *
 * StrBoWare is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StrBoWare.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>

#include "hexdump.h"

void hexdump_to_log(enum MessageVerboseLevel level,
                    const uint8_t *const buffer, size_t buffer_length,
                    const char *what)
{
    if(!msg_is_verbose(level))
        return;

    msg_vinfo(level, "%s (%zu bytes):", what, buffer_length);

    if(buffer == NULL || buffer_length == 0)
        return;

    static const size_t num_of_columns = 16;
    char log_buffer[1024];
    size_t out_pos = 0;

    for(size_t i = 0; i < buffer_length; ++i)
    {
        if((i % num_of_columns) == 0)
            out_pos += snprintf(log_buffer + out_pos, sizeof(log_buffer) - out_pos,
                                "0x%04lx:", (unsigned long)i);

        out_pos += snprintf(log_buffer + out_pos, sizeof(log_buffer) - out_pos, " 0x%02x", buffer[i]);

        if((i % num_of_columns) == num_of_columns - 1)
        {
            msg_vinfo(level, "%s", log_buffer);
            out_pos = 0;
        }
    }

    if(out_pos > 0)
        msg_vinfo(level, "%s", log_buffer);
}
