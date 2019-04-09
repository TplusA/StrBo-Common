/*
 * Copyright 2019  T+A elektroakustik GmbH & Co. KG
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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

    /* Any 32-bit or wider unsigned integer data type will do */
    using u32plus = unsigned int;

    struct Context
    {
        u32plus lo, hi;
        u32plus a, b, c, d;
        unsigned char buffer[64];
        u32plus block[16];
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
