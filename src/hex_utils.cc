/*
 * Copyright (C) 2024  T+A elektroakustik GmbH & Co. KG
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

#include "hex_utils.hh"

static char nibble_to_char(uint8_t nibble)
{
    if(nibble < 10)
        return '0' + nibble;
    else
        return 'a' + nibble - 10;
}

std::string StrBoUtils::binary_to_hexdump(const uint8_t *bin, size_t len)
{
    std::string dest;

    for(size_t i = 0; i < len; ++i)
    {
        const auto byte = bin[i];
        dest += nibble_to_char((byte >> 4) & 0x0f);
        dest += nibble_to_char(byte & 0x0f);
    }

    return dest;
}

std::string StrBoUtils::binary_to_hexdump(const std::string &bin)
{
    std::string dest;

    for(const auto &byte : bin)
    {
        dest += nibble_to_char((byte >> 4) & 0x0f);
        dest += nibble_to_char(byte & 0x0f);
    }

    return dest;
}

static uint8_t char_to_nibble(char ch)
{
    if(ch >= '0' && ch <= '9')
        return ch - '0';

    if(ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;

    if(ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;

    return UINT8_MAX;
}

size_t StrBoUtils::hexdump_to_binary(uint8_t *dest, size_t dest_size,
                                     const std::string &src)
{
    size_t j = 0;

    for(size_t i = 0; i < src.size() && j < dest_size; i += 2)
    {
        dest[j] = char_to_nibble(src[i]) << 4;

        if(i + 1 >= src.size())
            break;

        dest[j++] |= char_to_nibble(src[i + 1]);
    }

    return j;
}
