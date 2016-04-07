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

#include <cppcutter.h>

#include "stream_id.hh"

/*!
 * \addtogroup streamid_tests Unit tests
 * \ingroup streamid
 *
 * Stream ID definitions unit tests.
 */
/*!@{*/

static constexpr const stream_id_t our_source = STREAM_ID_MAKE_SOURCE(123);
static constexpr const stream_id_t other_source = STREAM_ID_MAKE_SOURCE(42);

namespace generic_stream_id_tests
{

void test_basic_constraints()
{
    cppcut_assert_operator(0U, <, STREAM_ID_SOURCE_BITS);
    cppcut_assert_operator(16U, >, STREAM_ID_SOURCE_BITS);

    cppcut_assert_operator(0U, <, STREAM_ID_SOURCE_SHIFT);
    cppcut_assert_operator(16U, >, STREAM_ID_SOURCE_SHIFT);

    cppcut_assert_not_equal(STREAM_ID_TYPE_CAST(0), STREAM_ID_SOURCE_MASK);
    cppcut_assert_not_equal(STREAM_ID_TYPE_CAST(0), STREAM_ID_COOKIE_MASK);
    cppcut_assert_equal(UINT16_MAX,
                        STREAM_ID_SOURCE_MASK | STREAM_ID_COOKIE_MASK);

    cppcut_assert_operator(0, <, STREAM_ID_COOKIE_MIN);
    cppcut_assert_operator(UINT16_MAX, >, STREAM_ID_COOKIE_MAX);

    cppcut_assert_equal(STREAM_ID_TYPE_CAST(0x0080), STREAM_ID_SOURCE_MIN);
    cppcut_assert_equal(STREAM_ID_TYPE_CAST(0xff80), STREAM_ID_SOURCE_MAX);
    cppcut_assert_equal(STREAM_ID_TYPE_CAST(0x0000), STREAM_ID_SOURCE_INVALID);
    cppcut_assert_equal(STREAM_ID_TYPE_CAST(0x0080), STREAM_ID_SOURCE_UI);
    cppcut_assert_equal(STREAM_ID_TYPE_CAST(0x0100), STREAM_ID_SOURCE_APP);
}

void test_op_less_than()
{
    auto a(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 10));
    auto b(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 20));

    cut_assert_false(a < a);
    cut_assert_true(a < b);
    cut_assert_false(b < a);
}

void test_op_greater_than()
{
    auto a(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 10));
    auto b(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 20));

    cut_assert_false(a > a);
    cut_assert_false(a > b);
    cut_assert_true(b > a);
}

void test_op_equals()
{
    auto a(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 10));
    auto b(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 20));

    cut_assert_true(a == a);
    cut_assert_false(a == b);
    cut_assert_false(b == a);
}

void test_op_unequal()
{
    auto a(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 10));
    auto b(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 20));

    cut_assert_false(a != a);
    cut_assert_true(a != b);
    cut_assert_true(b != a);
}

void test_make_source_macro()
{
    cppcut_assert_equal(STREAM_ID_TYPE_CAST(0x0000), STREAM_ID_MAKE_SOURCE(0x0000));
    cppcut_assert_equal(STREAM_ID_TYPE_CAST(0x0080), STREAM_ID_MAKE_SOURCE(0x0001));
    cppcut_assert_equal(STREAM_ID_TYPE_CAST(0x0b80), STREAM_ID_MAKE_SOURCE(0x0017));
    cppcut_assert_equal(STREAM_ID_TYPE_CAST(0xff00), STREAM_ID_MAKE_SOURCE(0x01fe));
    cppcut_assert_equal(STREAM_ID_TYPE_CAST(0xff80), STREAM_ID_MAKE_SOURCE(0x01ff));
    cppcut_assert_equal(STREAM_ID_TYPE_CAST(0x0000), STREAM_ID_MAKE_SOURCE(0x0200));
}

void test_initial_stream_id_is_minimum_defined_value()
{
    auto id(ID::Stream::make_for_source(our_source));

    cppcut_assert_equal(STREAM_ID_COOKIE_MIN, id.get_cookie());
    cppcut_assert_equal(our_source, id.get_source());
}

void test_increment_by_one()
{
    auto id(ID::Stream::make_for_source(our_source));

    auto expected_cookie = id.get_cookie();
    cppcut_assert_equal(expected_cookie, id.get_cookie());

    ++expected_cookie;
    ++id;
    cppcut_assert_equal(expected_cookie, id.get_cookie());
}

void test_increment_beyond_max()
{
    auto id(ID::Stream::make_for_source(STREAM_ID_SOURCE_MAX));

    auto expected_cookie = id.get_cookie();
    cppcut_assert_equal(expected_cookie, id.get_cookie());

    for(stream_id_t i = 0; i < STREAM_ID_COOKIE_MAX - 1; ++i)
    {
        ++expected_cookie;
        ++id;
        cppcut_assert_equal(expected_cookie, id.get_cookie());
        cppcut_assert_equal(STREAM_ID_SOURCE_MAX, id.get_source());
    }

    cppcut_assert_equal(STREAM_ID_COOKIE_MAX, id.get_cookie());

    ++id;
    cppcut_assert_equal(STREAM_ID_COOKIE_MIN, id.get_cookie());
    cppcut_assert_equal(STREAM_ID_SOURCE_MAX, id.get_source());
}

void test_all_components_of_invalid_id_are_invalid()
{
    auto id(ID::Stream::make_invalid());

    cut_assert_false(id.is_valid());
    cppcut_assert_equal(STREAM_ID_SOURCE_INVALID, id.get_source());
    cppcut_assert_equal(STREAM_ID_COOKIE_INVALID, id.get_cookie());
}

}

namespace sourced_stream_id_tests
{

void test_id_contains_source_id()
{
    auto id(ID::SourcedStream<our_source>::make());

    cut_assert_true(id.get().is_valid());
    cut_assert_true(ID::SourcedStream<our_source>::compatible_with(id.get()));
    cppcut_assert_equal(our_source, id.get().get_source());
    cppcut_assert_equal(STREAM_ID_COOKIE_MIN, id.get().get_cookie());
}

void test_id_with_start_cookie_contains_source_id()
{
    auto id(ID::SourcedStream<our_source>::make(5));

    cut_assert_true(id.get().is_valid());
    cppcut_assert_equal(our_source, id.get().get_source());
    cppcut_assert_equal(STREAM_ID_TYPE_CAST(5), id.get().get_cookie());
}

void test_invalid_id_contains_source_id()
{
    auto id(ID::SourcedStream<our_source>::make_invalid());

    cut_assert_false(id.get().is_valid());
    cut_assert_true(ID::SourcedStream<our_source>::compatible_with(id.get()));
    cut_assert_false(ID::SourcedStream<our_source>::compatible_with(ID::Stream::make_invalid()));
    cppcut_assert_equal(our_source, id.get().get_source());
    cppcut_assert_equal(STREAM_ID_COOKIE_INVALID, id.get().get_cookie());
}

void test_conversion_from_generic_id_with_matching_source_id()
{
    auto generic_id(ID::Stream::make_complete(our_source, 80));
    auto our_id(ID::SourcedStream<our_source>::make_from_generic_id(generic_id));

    cut_assert_true(generic_id.is_valid());
    cut_assert_true(our_id.get().is_valid());
    cppcut_assert_equal(our_source, our_id.get().get_source());
    cppcut_assert_equal(STREAM_ID_TYPE_CAST(80), our_id.get().get_cookie());
}

void test_conversion_from_generic_id_with_mismatching_source_id()
{
    auto generic_id(ID::Stream::make_complete(STREAM_ID_MAKE_SOURCE(42), 90));
    auto our_id(ID::SourcedStream<our_source>::make_from_generic_id(generic_id));

    cut_assert_true(generic_id.is_valid());
    cut_assert_false(our_id.get().is_valid());
    cppcut_assert_equal(our_source, our_id.get().get_source());
    cppcut_assert_equal(STREAM_ID_COOKIE_INVALID, our_id.get().get_cookie());
}

void test_conversion_from_generic_invalid_id_replaces_source_id()
{
    auto a(ID::Stream::make_invalid());
    auto b(ID::Stream::make_complete(STREAM_ID_MAKE_SOURCE(42), STREAM_ID_COOKIE_INVALID));

    cut_assert_false(a.is_valid());
    cut_assert_false(b.is_valid());

    auto our_a(ID::SourcedStream<our_source>::make_from_generic_id(a));

    cut_assert_false(our_a.get().is_valid());
    cppcut_assert_equal(our_source, our_a.get().get_source());
    cppcut_assert_equal(STREAM_ID_COOKIE_INVALID, our_a.get().get_cookie());

    auto our_b(ID::SourcedStream<our_source>::make_from_generic_id(b));

    cut_assert_false(our_b.get().is_valid());
    cppcut_assert_equal(our_source, our_b.get().get_source());
    cppcut_assert_equal(STREAM_ID_COOKIE_INVALID, our_b.get().get_cookie());
}

void test_id_from_different_sources_are_incompatible()
{
    auto our_id_a(ID::Stream::make_complete(our_source, 80));
    auto our_id_b(ID::Stream::make_complete(our_source, 81));
    auto other_id_a(ID::Stream::make_complete(other_source, 90));
    auto other_id_b(ID::Stream::make_complete(other_source, 91));

    cut_assert_true(ID::SourcedStream<our_source>::compatible_with(our_id_a));
    cut_assert_true(ID::SourcedStream<our_source>::compatible_with(our_id_b));
    cut_assert_false(ID::SourcedStream<our_source>::compatible_with(other_id_a));
    cut_assert_false(ID::SourcedStream<our_source>::compatible_with(other_id_b));

    cut_assert_false(ID::SourcedStream<other_source>::compatible_with(our_id_a));
    cut_assert_false(ID::SourcedStream<other_source>::compatible_with(our_id_b));
    cut_assert_true(ID::SourcedStream<other_source>::compatible_with(other_id_a));
    cut_assert_true(ID::SourcedStream<other_source>::compatible_with(other_id_b));
}

}

/*!@}*/
