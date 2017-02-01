/*
 * Copyright (C) 2015, 2017  T+A elektroakustik GmbH & Co. KG
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
 * along with StrBoWare  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MD5_HH
#define MD5_HH

#include <string>
#include <array>
#include <cstdint>

/*!
 * \addtogroup md5 MD5 digest computation
 */
/*!@{*/

namespace MD5
{
    using Hash = std::array<uint8_t, 16>;

    struct Context
    {
        uint32_t i[2];
        uint32_t buf[4];
        uint8_t in[64];
    };

    void init(Context &ctx);

    void update(Context &ctx, const uint8_t *data, size_t data_size);

    static inline void update(Context &ctx, const std::string &data)
    {
        update(ctx,
               static_cast<const uint8_t *>(static_cast<const void *>(data.c_str())),
               data.length());
    }

    void finish(Context &ctx, Hash &hash);

    void to_string(const Hash &hash, std::string &hash_string);
}

/*!@}*/

#endif /* !MD5_HH */