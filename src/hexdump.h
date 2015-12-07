/*
 * Copyright (C) 2016, 2019  T+A elektroakustik GmbH & Co. KG
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

#ifndef HEXDUMP_H
#define HEXDUMP_H

#include <stdint.h>

#include "messages.h"

#ifdef __cplusplus
extern "C" {
#endif

void hexdump_to_log(enum MessageVerboseLevel level,
                    const uint8_t *const buffer, size_t buffer_length,
                    const char *what);

#ifdef __cplusplus
}
#endif

#endif /* !HEXDUMP_H */
