/*
 * Copyright (C) 2018  T+A elektroakustik GmbH & Co. KG
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

#include <doctest.h>

#include "stream_id.hh"

#include <iostream>

/*!
 * \addtogroup streamid_tests Unit tests
 * \ingroup streamid
 *
 * Stream ID definitions unit tests.
 */
/*!@{*/

static constexpr const stream_id_t our_source = STREAM_ID_MAKE_SOURCE(123);
static constexpr const stream_id_t other_source = STREAM_ID_MAKE_SOURCE(42);

class GenericStreamIDTests
{
  public:
    explicit GenericStreamIDTests() {}
};

TEST_CASE_FIXTURE(GenericStreamIDTests, "Basic constraints")
{
    CHECK(STREAM_ID_SOURCE_BITS > 0);
    CHECK(STREAM_ID_SOURCE_BITS < 16);

    CHECK(STREAM_ID_SOURCE_SHIFT > 0);
    CHECK(STREAM_ID_SOURCE_SHIFT < 16);

    CHECK(STREAM_ID_SOURCE_MASK != STREAM_ID_TYPE_CAST(0));
    CHECK(STREAM_ID_COOKIE_MASK != STREAM_ID_TYPE_CAST(0));
    CHECK((STREAM_ID_SOURCE_MASK | STREAM_ID_COOKIE_MASK) == UINT16_MAX);

    CHECK(STREAM_ID_COOKIE_MIN > 0);
    CHECK(STREAM_ID_COOKIE_MAX < UINT16_MAX);

    CHECK(STREAM_ID_SOURCE_MIN == STREAM_ID_TYPE_CAST(0x0080));
    CHECK(STREAM_ID_SOURCE_MAX == STREAM_ID_TYPE_CAST(0xff80));
    CHECK(STREAM_ID_SOURCE_INVALID == STREAM_ID_TYPE_CAST(0x0000));
    CHECK(STREAM_ID_SOURCE_UI == STREAM_ID_TYPE_CAST(0x0080));
    CHECK(STREAM_ID_SOURCE_APP == STREAM_ID_TYPE_CAST(0x0100));
}

TEST_CASE_FIXTURE(GenericStreamIDTests, "Operator <")
{
    auto a(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 10));
    auto b(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 20));

    CHECK_FALSE(a < a);
    CHECK(a < b);
    CHECK_FALSE(b < a);
}

TEST_CASE_FIXTURE(GenericStreamIDTests, "Operator >")
{
    auto a(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 10));
    auto b(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 20));

    CHECK_FALSE(a > a);
    CHECK_FALSE(a > b);
    CHECK(b > a);
}

TEST_CASE_FIXTURE(GenericStreamIDTests, "Operator ==")
{
    auto a(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 10));
    auto b(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 20));

    CHECK(a == a);
    CHECK_FALSE(a == b);
    CHECK_FALSE(b == a);
}

TEST_CASE_FIXTURE(GenericStreamIDTests, "Operator !=")
{
    auto a(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 10));
    auto b(ID::Stream::make_complete(our_source, STREAM_ID_SOURCE_MIN + 20));

    CHECK_FALSE(a != a);
    CHECK(a != b);
    CHECK(b != a);
}

TEST_CASE_FIXTURE(GenericStreamIDTests, "Make source macro")
{
    CHECK(STREAM_ID_MAKE_SOURCE(0x0000) == STREAM_ID_TYPE_CAST(0x0000));
    CHECK(STREAM_ID_MAKE_SOURCE(0x0001) == STREAM_ID_TYPE_CAST(0x0080));
    CHECK(STREAM_ID_MAKE_SOURCE(0x0017) == STREAM_ID_TYPE_CAST(0x0b80));
    CHECK(STREAM_ID_MAKE_SOURCE(0x01fe) == STREAM_ID_TYPE_CAST(0xff00));
    CHECK(STREAM_ID_MAKE_SOURCE(0x01ff) == STREAM_ID_TYPE_CAST(0xff80));
    CHECK(STREAM_ID_MAKE_SOURCE(0x0200) == STREAM_ID_TYPE_CAST(0x0000));
}

TEST_CASE_FIXTURE(GenericStreamIDTests, "Initial stream ID is minimum defined value")
{
    auto id(ID::Stream::make_for_source(our_source));

    CHECK(id.get_cookie() == STREAM_ID_COOKIE_MIN);
    CHECK(id.get_source() == our_source);
}

TEST_CASE_FIXTURE(GenericStreamIDTests, "Increment ID by one")
{
    auto id(ID::Stream::make_for_source(our_source));

    auto expected_cookie = id.get_cookie();
    CHECK(id.get_cookie() == expected_cookie);

    ++expected_cookie;
    ++id;
    CHECK(id.get_cookie() == expected_cookie);
}

TEST_CASE_FIXTURE(GenericStreamIDTests, "Increment ID beyond maximum")
{
    auto id(ID::Stream::make_for_source(STREAM_ID_SOURCE_MAX));

    auto expected_cookie = id.get_cookie();
    CHECK(id.get_cookie() == expected_cookie);

    for(stream_id_t i = 0; i < STREAM_ID_COOKIE_MAX - 1; ++i)
    {
        ++expected_cookie;
        ++id;
        CHECK(id.get_cookie() == expected_cookie);
        CHECK(id.get_source() == STREAM_ID_SOURCE_MAX);
    }

    CHECK(id.get_cookie() == STREAM_ID_COOKIE_MAX);

    ++id;
    CHECK(id.get_cookie() == STREAM_ID_COOKIE_MIN);
    CHECK(id.get_source() == STREAM_ID_SOURCE_MAX);
}

TEST_CASE_FIXTURE(GenericStreamIDTests, "All components of invalid ID are invalid")
{
    auto id(ID::Stream::make_invalid());

    CHECK_FALSE(id.is_valid());
    CHECK(id.get_source() == STREAM_ID_SOURCE_INVALID);
    CHECK(id.get_cookie() == STREAM_ID_COOKIE_INVALID);
}


class SourcedStreamIDTests
{
  public:
    explicit SourcedStreamIDTests() {}
};

TEST_CASE_FIXTURE(SourcedStreamIDTests, "ID contains source ID")
{
    auto id(ID::SourcedStream<our_source>::make());

    CHECK(id.get().is_valid());
    CHECK(ID::SourcedStream<our_source>::compatible_with(id.get()));
    CHECK(id.get().get_source() == our_source );
    CHECK(id.get().get_cookie() == STREAM_ID_COOKIE_MIN);
}

TEST_CASE_FIXTURE(SourcedStreamIDTests, "Id with start cookie contains source ID")
{
    auto id(ID::SourcedStream<our_source>::make(5));

    CHECK(id.get().is_valid());
    CHECK(id.get().get_source() == our_source);
    CHECK(id.get().get_cookie() == STREAM_ID_TYPE_CAST(5));
}

TEST_CASE_FIXTURE(SourcedStreamIDTests, "Invalid ID contains source ID")
{
    auto id(ID::SourcedStream<our_source>::make_invalid());

    CHECK_FALSE(id.get().is_valid());
    CHECK(ID::SourcedStream<our_source>::compatible_with(id.get()));
    CHECK_FALSE(ID::SourcedStream<our_source>::compatible_with(ID::Stream::make_invalid()));
    CHECK(id.get().get_source() == our_source);
    CHECK(id.get().get_cookie() == STREAM_ID_COOKIE_INVALID);
}

TEST_CASE_FIXTURE(SourcedStreamIDTests, "Conversion from generic ID with matching source ID")
{
    auto generic_id(ID::Stream::make_complete(our_source, 80));
    auto our_id(ID::SourcedStream<our_source>::make_from_generic_id(generic_id));

    CHECK(generic_id.is_valid());
    CHECK(our_id.get().is_valid());
    CHECK(our_id.get().get_source() == our_source);
    CHECK(our_id.get().get_cookie() == STREAM_ID_TYPE_CAST(80));
}

TEST_CASE_FIXTURE(SourcedStreamIDTests, "Conversion from generic ID with mismatching source ID")
{
    auto generic_id(ID::Stream::make_complete(STREAM_ID_MAKE_SOURCE(42), 90));
    auto our_id(ID::SourcedStream<our_source>::make_from_generic_id(generic_id));

    CHECK(generic_id.is_valid());
    CHECK_FALSE(our_id.get().is_valid());
    CHECK(our_id.get().get_source() == our_source);
    CHECK(our_id.get().get_cookie() == STREAM_ID_COOKIE_INVALID);
}

TEST_CASE_FIXTURE(SourcedStreamIDTests, "Conversion from generic invalid ID replaces source ID")
{
    auto a(ID::Stream::make_invalid());
    auto b(ID::Stream::make_complete(STREAM_ID_MAKE_SOURCE(42), STREAM_ID_COOKIE_INVALID));

    CHECK_FALSE(a.is_valid());
    CHECK_FALSE(b.is_valid());

    auto our_a(ID::SourcedStream<our_source>::make_from_generic_id(a));

    CHECK_FALSE(our_a.get().is_valid());
    CHECK(our_a.get().get_source() == our_source);
    CHECK(our_a.get().get_cookie() == STREAM_ID_COOKIE_INVALID);

    auto our_b(ID::SourcedStream<our_source>::make_from_generic_id(b));

    CHECK_FALSE(our_b.get().is_valid());
    CHECK(our_b.get().get_source() == our_source);
    CHECK(our_b.get().get_cookie() == STREAM_ID_COOKIE_INVALID);
}

TEST_CASE_FIXTURE(SourcedStreamIDTests, "ID from different sources are incompatible")
{
    auto our_id_a(ID::Stream::make_complete(our_source, 80));
    auto our_id_b(ID::Stream::make_complete(our_source, 81));
    auto other_id_a(ID::Stream::make_complete(other_source, 90));
    auto other_id_b(ID::Stream::make_complete(other_source, 91));

    CHECK(ID::SourcedStream<our_source>::compatible_with(our_id_a));
    CHECK(ID::SourcedStream<our_source>::compatible_with(our_id_b));
    CHECK_FALSE(ID::SourcedStream<our_source>::compatible_with(other_id_a));
    CHECK_FALSE(ID::SourcedStream<our_source>::compatible_with(other_id_b));

    CHECK_FALSE(ID::SourcedStream<other_source>::compatible_with(our_id_a));
    CHECK_FALSE(ID::SourcedStream<other_source>::compatible_with(our_id_b));
    CHECK(ID::SourcedStream<other_source>::compatible_with(other_id_a));
    CHECK(ID::SourcedStream<other_source>::compatible_with(other_id_b));
}

/*!@}*/
