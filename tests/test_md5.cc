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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <algorithm>
#include <cppcutter.h>

#include "md5.hh"

/*!
 * \addtogroup md5_tests Unit tests
 * \ingroup md5
 *
 * Unit tests for MD5 digest computation.
 */
/*!@{*/

namespace md5_tests
{

static MD5::Context context;
static MD5::Hash hash;

void cut_setup()
{
    MD5::init(context);
}

static constexpr MD5::Hash expected_hash_for_empty_input
{
    0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
    0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e
};

void test_empty_input()
{
    MD5::finish(context, hash);

    cut_assert_equal_memory(expected_hash_for_empty_input.data(),
                            expected_hash_for_empty_input.size(),
                            hash.data(), hash.size());
}

void test_empty_ascii_string()
{
    const std::string input_data;

    MD5::update(context, input_data);
    MD5::finish(context, hash);

    cut_assert_equal_memory(expected_hash_for_empty_input.data(),
                            expected_hash_for_empty_input.size(),
                            hash.data(), hash.size());
}

void test_short_binary_input_single_update()
{
    static constexpr uint8_t input_data[] =
    {
        0xed, 0xd6, 0xda, 0x86, 0xec, 0x3d, 0x08, 0xd9,
        0x45, 0xf4, 0xe4, 0x3b, 0x9f, 0xbf, 0x77, 0x29,
        0x1e, 0x07, 0xb8, 0xd4
    };

    static constexpr MD5::Hash expected_hash
    {
        0x8b, 0xaa, 0xe2, 0xe2, 0xc2, 0xa4, 0xab, 0x23,
        0x95, 0xd5, 0x6c, 0xbd, 0xbc, 0x0d, 0x6a, 0x95
    };

    MD5::update(context, input_data, sizeof(input_data));
    MD5::finish(context, hash);

    cut_assert_equal_memory(expected_hash.data(), expected_hash.size(),
                            hash.data(), hash.size());
}

static constexpr MD5::Hash expected_hash_short_ascii_string
{
    0xbf, 0x00, 0x67, 0x8a, 0x76, 0x1b, 0x0d, 0x19,
    0x20, 0x93, 0xeb, 0x80, 0xec, 0x63, 0x22, 0x21
};

void test_short_ascii_string_single_update()
{
    const std::string input_data = "short test string";

    MD5::update(context, input_data);
    MD5::finish(context, hash);

    cut_assert_equal_memory(expected_hash_short_ascii_string.data(),
                            expected_hash_short_ascii_string.size(),
                            hash.data(), hash.size());
}

void test_short_ascii_string_multiple_updates()
{
    const std::array<const std::string, 5> input_data
    {
        "short",
        " ",
        "test",
        " ",
        "string"
    };

    for(const auto &s : input_data)
        MD5::update(context, s);

    MD5::finish(context, hash);

    cut_assert_equal_memory(expected_hash_short_ascii_string.data(),
                            expected_hash_short_ascii_string.size(),
                            hash.data(), hash.size());
}

static constexpr uint8_t big_binary_input_data[] =
{
    0xed, 0x01, 0x6f, 0xe8, 0x1f, 0x8d, 0x9d, 0xd9, 0x34, 0xa2, 0x25, 0xee, 0xf2, 0x59, 0x45, 0x4f,
    0xa0, 0x0b, 0x94, 0xb2, 0x90, 0xc7, 0x7b, 0xd4, 0x5f, 0xce, 0xf8, 0x41, 0x3b, 0x8b, 0x97, 0x76,
    0xf2, 0xc1, 0x85, 0x27, 0x97, 0xd8, 0x21, 0x81, 0xe8, 0x3f, 0x28, 0x7a, 0xdf, 0x23, 0xb9, 0xcd,
    0x8d, 0xd0, 0x0b, 0x7e, 0x44, 0xac, 0x46, 0x56, 0x60, 0x2e, 0xb9, 0x41, 0x05, 0xc7, 0x74, 0xf1,
    0x57, 0xde, 0xed, 0x68, 0x22, 0x11, 0xe4, 0xe5, 0x4e, 0x80, 0xa5, 0xac, 0xa4, 0x9a, 0x2b, 0xfb,
    0xc5, 0x9a, 0x4c, 0xae, 0x34, 0x52, 0xdf, 0xcf, 0xf4, 0xa9, 0x73, 0x59, 0xb0, 0x93, 0x53, 0xf1,
    0xe1, 0x1a, 0x2b, 0x84, 0xfd, 0x3d, 0x0c, 0xd3, 0xf3, 0xe7, 0xc2, 0xa9, 0x5b, 0x86, 0x60, 0x53,
    0x93, 0x72, 0x12, 0x32, 0x2e, 0xc8, 0xa0, 0x7e, 0x44, 0xeb, 0xa8, 0x6f, 0x37, 0xd8, 0x39, 0x9d,
    0xc1, 0xd7, 0xe5, 0xe5, 0xe8, 0xef, 0x21, 0x0f, 0xde, 0x34, 0x70, 0xf3, 0xe5, 0x87, 0x9b, 0x15,
    0x36, 0x1b, 0x5a, 0x6b, 0xbb, 0xff, 0x4c, 0xa5, 0xa7, 0xa9, 0x77, 0x16, 0x85, 0x83, 0x8f, 0x41,
    0xa7, 0x7f, 0xcf, 0xc1, 0x20, 0x53, 0xb9, 0x92, 0x5b, 0x1f, 0x31, 0xb3, 0x4b, 0x30, 0x05, 0xdc,
    0x00, 0x4d, 0x0a, 0x85, 0x1b, 0x15, 0x40, 0xaa, 0xf3, 0xcd, 0xf0, 0x79, 0x09, 0xdc, 0x21, 0x4c,
    0x4c, 0xb8, 0x53, 0x3c, 0x0d, 0xe9, 0xde, 0xdc, 0x7e, 0x9e, 0xfa, 0x6d, 0x94, 0xaa, 0xcd, 0xfd,
    0x79, 0x39, 0x74, 0x1a, 0x93, 0x80, 0xa8, 0xbb, 0xc2, 0x49, 0xcd, 0x63, 0x85, 0x81, 0x78, 0x93,
    0x3d, 0x04, 0x60, 0xdd, 0xb9, 0xe6, 0x37, 0x7a, 0x3a, 0x3a, 0x28, 0x6d, 0xca, 0x1e, 0xb0, 0xaa,
    0xae, 0x20, 0x13, 0x8b, 0xa2, 0x36, 0xc0, 0xdc, 0x70, 0xe5, 0xa2, 0x0f, 0xce, 0x6e, 0xaf, 0x44,
    0x82, 0x97, 0x4b, 0xc1, 0xb3, 0x35, 0xc4, 0xbd, 0x42, 0x00, 0xc2, 0x95, 0xd1, 0x89, 0xeb, 0xad,
    0x83, 0x05, 0xb3, 0xfb, 0x45, 0x1d, 0x8b, 0xa1, 0x2b, 0x87, 0xcd, 0x37, 0x37, 0xd1, 0x56, 0xa4,
    0xde, 0x8c, 0x53, 0x93, 0x92, 0xef, 0xbb, 0xcc, 0xc4, 0x28, 0x72, 0xd1, 0xc7, 0xfd, 0x3f, 0xb7,
    0xa2, 0x94, 0xc8, 0xa5, 0x6c, 0x35, 0x8d, 0x5f, 0x71, 0x7f, 0x8e, 0xa5, 0xf3, 0x78, 0xd1, 0x04,
    0xfb, 0x9a, 0x32, 0xf4, 0x5b, 0x53, 0xe8, 0x91, 0x70, 0xa9, 0x40, 0x26, 0x1d, 0xb4, 0x53, 0x5d,
    0x07, 0x68, 0x7a, 0xc3, 0x77, 0x9b, 0xf5, 0xb2, 0x3f, 0x3f, 0x5c, 0x56, 0x32, 0x61, 0xa7, 0xe5,
    0x65, 0xc0, 0x63, 0x70, 0x29, 0x3f, 0x2e, 0xaa, 0x2e, 0x78, 0x2e, 0xab, 0xe1, 0xe1, 0x08, 0x6c,
    0x87, 0x56, 0xb7, 0x39, 0xa6, 0xa4, 0x83, 0x9c, 0x32, 0x47, 0x66, 0xd5, 0xfa, 0xb5, 0xde, 0x11,
    0xd3, 0x47, 0xa3, 0xcb, 0x9c, 0xdb, 0xd1, 0x4a, 0x7d, 0xa5, 0x40, 0x17, 0x8a, 0xb4, 0x3f, 0x32,
    0x15, 0x15, 0xdd, 0xc3, 0x26, 0xd7, 0x9c, 0x0d, 0xc5, 0xd2, 0x6a, 0xf3, 0x11, 0x20, 0x4d, 0xcf,
    0x8b, 0x5e, 0x01, 0xff, 0x65, 0x31, 0x43, 0x3e, 0xc5, 0x24, 0xe1, 0x96, 0x12, 0x94, 0x94, 0xe2,
    0x38, 0x05, 0xe6, 0x66, 0xe8, 0xa8, 0xd0, 0x49, 0xe6, 0x4a, 0xd4, 0x02, 0xa1, 0x59, 0x9a, 0x8f,
    0xed, 0x71, 0x53, 0x79, 0xa7, 0xc7, 0xf0, 0xee, 0x51, 0x64, 0xa2, 0xee, 0x59, 0x4f, 0xfb, 0xef,
    0x6e, 0x44, 0x54, 0x5e, 0x1a, 0x93, 0x11, 0x4e, 0xe6, 0xfd, 0x6a, 0xc8, 0xee, 0x56, 0x4d, 0x36,
    0x5b, 0xcc, 0x4f, 0x9b, 0xa9, 0x0e, 0xb0, 0xbd, 0xa8, 0xf8, 0x09, 0xd3, 0x90, 0x41, 0xbe, 0x7c,
    0x73, 0xe3, 0x8b, 0xc1, 0xd9, 0x9f, 0x24, 0xd1, 0xf3, 0xa4, 0x63, 0x87, 0x75, 0x92, 0xbc, 0xab,
};

static constexpr MD5::Hash expected_hash_big_binary_data
{
    0xce, 0x81, 0xad, 0xc3, 0x75, 0xaf, 0x11, 0xe2,
    0x8d, 0xa3, 0x8c, 0xda, 0x7d, 0x2b, 0xc8, 0x30
};

void test_big_binary_input_single_update()
{
    MD5::update(context, big_binary_input_data, sizeof(big_binary_input_data));
    MD5::finish(context, hash);

    cut_assert_equal_memory(expected_hash_big_binary_data.data(),
                            expected_hash_big_binary_data.size(),
                            hash.data(), hash.size());
}

void test_big_binary_input_multiple_updates()
{
    static constexpr size_t max_chunk_size = 20;

    for(size_t i = 0; i < sizeof(big_binary_input_data); i += max_chunk_size)
    {
        const size_t chunk_size =
            std::min(max_chunk_size, sizeof(big_binary_input_data) - i);

        MD5::update(context, big_binary_input_data + i, chunk_size);
    }

    MD5::finish(context, hash);

    cut_assert_equal_memory(expected_hash_big_binary_data.data(),
                            expected_hash_big_binary_data.size(),
                            hash.data(), hash.size());
}

void test_conversion_to_string()
{
    const std::string expected_string = "e4fbe92736fbe6d5913e8a5f3cf64470";
    static constexpr MD5::Hash input_hash
    {
        0xe4, 0xfb, 0xe9, 0x27, 0x36, 0xfb, 0xe6, 0xd5,
        0x91, 0x3e, 0x8a, 0x5f, 0x3c, 0xf6, 0x44, 0x70
    };

    std::string output;
    MD5::to_string(input_hash, output);
    cppcut_assert_equal(expected_string, output);
}

};

/*!@}*/
