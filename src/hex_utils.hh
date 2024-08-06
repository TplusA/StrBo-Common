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

#ifndef HEX_UTILS_HH
#define HEX_UTILS_HH

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace StrBoUtils
{

std::string binary_to_hexdump(const uint8_t *bin, size_t len);
std::string binary_to_hexdump(const std::string &bin);
size_t hexdump_to_binary(uint8_t *dest, size_t dest_size, const std::string &src);
std::vector<uint8_t> hexdump_to_binary(const std::string &src);

}

#endif /* !HEX_UTILS_HH */
