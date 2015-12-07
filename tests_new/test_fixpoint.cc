/*
 * Copyright (C) 2017, 2019  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of DCPD.
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

#include <doctest.h>
#include <iomanip>
#include <random>

#include "fixpoint.hh"

TEST_SUITE_BEGIN("14-bit fix point format tests");

const int16_t FixPoint::MIN_AS_INT16;
const int16_t FixPoint::MAX_AS_INT16;

static void expect_equal_doubles(double value, double expected)
{
    if(std::isnan(expected) || std::isnan(value))
    {
        if(std::isnan(expected) && std::isnan(value))
            return;

        FAIL("Expected " << std::setprecision(20) << expected << ", got " << value);
    }

    {
        std::ostringstream os;
        os << "Expected " << std::setprecision(20) << value << " <= " << expected;
        const auto s(os.str());
        CAPTURE(s);
        CHECK_MESSAGE(value <= expected, s);
    }

    {
        std::ostringstream os;
        os << "Expected " << std::setprecision(20) << value << " >= " << expected;
        const auto s(os.str());
        CAPTURE(s);
        CHECK_MESSAGE(value >= expected, s);
    }
}

static void expect_equal_doubles(double value, double input, double expected)
{
    if(std::isnan(expected) || std::isnan(value))
    {
        if(std::isnan(expected) && std::isnan(value))
            return;

        FAIL("Failed for input " << std::setprecision(20) << input <<
             ": expected " << expected << ", got " << value);
    }

    {
        std::ostringstream os;
        os << "Failed for input " << std::setprecision(20) << input
           << ": expected " << value << " <= " << expected;
        const auto s(os.str());
        CAPTURE(s);
        CHECK_MESSAGE(value <= expected, s);
    }

    {
        std::ostringstream os;
        os << "Failed for input " << std::setprecision(20) << input
           << ": expected " << value << " >= " << expected;
        const auto s(os.str());
        CAPTURE(s);
        CHECK_MESSAGE(value >= expected, s);
    }
}

static void expect_conversion_result(const FixPoint &fp, int16_t expected)
{
    REQUIRE_FALSE(fp.is_nan());
    CHECK(fp.to_int16() == expected);
}

static void expect_conversion_result(const FixPoint &fp, double expected)
{
    REQUIRE_FALSE(fp.is_nan());

    const double fp_as_double = fp.to_double();
    expect_equal_doubles(fp_as_double, expected);
}

TEST_CASE("Integer 0")
{
    const auto value = FixPoint(int16_t(0));
    expect_conversion_result(value, int16_t(0));
}

TEST_CASE("Integer 1")
{
    const auto value = FixPoint(int16_t(1));
    expect_conversion_result(value, int16_t(1));
}

TEST_CASE("Integer -1")
{
    const auto value = FixPoint(int16_t(-1));
    expect_conversion_result(value, int16_t(-1));
}

TEST_CASE("Minimum int16_t value")
{
    const auto value = FixPoint(FixPoint::MIN_AS_INT16);
    expect_conversion_result(value, FixPoint::MIN_AS_INT16);
}

TEST_CASE("Maximum int16_t value")
{
    const auto value = FixPoint(FixPoint::MAX_AS_INT16);
    expect_conversion_result(value, FixPoint::MAX_AS_INT16);
}

TEST_CASE("Integer overflow generates NaN")
{
    const auto value = FixPoint(int16_t(FixPoint::MAX_AS_INT16 + 1));
    REQUIRE(value.is_nan());
    CHECK(value.to_int16() == int16_t(INT16_MIN));
    CHECK(std::isnan(value.to_double()));
}

TEST_CASE("Integer underflow generates NaN")
{
    const auto value = FixPoint(int16_t(FixPoint::MIN_AS_INT16 - 1));
    REQUIRE(value.is_nan());
    CHECK(value.to_int16() == int16_t(INT16_MIN));
    CHECK(std::isnan(value.to_double()));
}

TEST_CASE("Double 0.0")
{
    const auto value = FixPoint(0.0);
    expect_conversion_result(value, int16_t(0));
}

TEST_CASE("Double 1.0")
{
    const auto value = FixPoint(1.0);
    expect_conversion_result(value, int16_t(1));
}

TEST_CASE("Double -1.0")
{
    const auto value = FixPoint(-1.0);
    expect_conversion_result(value, int16_t(-1));
}

TEST_CASE("Double 3.75")
{
    const auto value = FixPoint(3.75);
    expect_conversion_result(value, 3.75);
}

TEST_CASE("Double -7.25")
{
    const auto value = FixPoint(-7.25);
    expect_conversion_result(value, -7.25);
}

TEST_CASE("Conversion from FixPoint to native types")
{
    const auto pos = FixPoint(10.5);

    expect_equal_doubles(pos.to_double(), 10.5);
    CHECK(pos.to_int16() == int16_t(11));

    const auto neg = FixPoint(-5.0625);

    expect_equal_doubles(neg.to_double(), -5.0625);
    CHECK(neg.to_int16() == int16_t(-5));
}

TEST_CASE("Serialization to buffer requires minimum buffer size")
{
    const auto value = FixPoint(-0.25);
    const std::array<const uint8_t, 8> expected_empty{0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
    const std::array<const uint8_t, 8> expected_filled{0x55, 0x55, 0x20, 0x04, 0x55, 0x55, 0x55, 0x55};

    std::array<uint8_t, 8> buffer;

    std::fill(buffer.begin(), buffer.end(), 0x55);
    CHECK_FALSE(value.to_buffer(buffer.data() + 2, 0));
    CHECK(std::equal(buffer.begin(), buffer.end(), expected_empty.begin()));

    std::fill(buffer.begin(), buffer.end(), 0x55);
    CHECK_FALSE(value.to_buffer(buffer.data() + 2, 1));
    CHECK(std::equal(buffer.begin(), buffer.end(), expected_empty.begin()));

    std::fill(buffer.begin(), buffer.end(), 0x55);
    CHECK(value.to_buffer(buffer.data() + 2, 2));
    CHECK(std::equal(buffer.begin(), buffer.end(), expected_filled.begin()));

    std::fill(buffer.begin(), buffer.end(), 0x55);
    CHECK(value.to_buffer(buffer.data() + 2, 3));
    CHECK(std::equal(buffer.begin(), buffer.end(), expected_filled.begin()));

    std::fill(buffer.begin(), buffer.end(), 0x55);
    CHECK(value.to_buffer(buffer.data() + 2, 4));
    CHECK(std::equal(buffer.begin(), buffer.end(), expected_filled.begin()));
}

static void expect_serialization_result(const FixPoint &value,
                                        const std::array<const uint8_t, 2> &expected)
{
    std::array<uint8_t, 2> buffer;
    CHECK(value.to_buffer(buffer.data(), buffer.size()));
    CHECK(std::equal(buffer.begin(), buffer.end(), expected.begin()));
}

TEST_CASE("Serialize 0 to buffer")
{
    const auto value = FixPoint(int16_t(0));
    expect_serialization_result(value, {0x00, 0x00});
}

TEST_CASE("Serialize 1 to buffer")
{
    const auto value = FixPoint(int16_t(1));
    expect_serialization_result(value, {0x00, 0x10});
}

TEST_CASE("Serialize 42 to buffer")
{
    const auto value = FixPoint(int16_t(42));
    expect_serialization_result(value, {0x02, 0xa0});
}

TEST_CASE("Serialize int16_t maximum value to buffer")
{
    const auto value = FixPoint(FixPoint::MAX_AS_INT16);
    expect_serialization_result(value, {0x1f, 0xf0});
}

TEST_CASE("Serialize -1 to buffer")
{
    const auto value = FixPoint(int16_t(-1));
    expect_serialization_result(value, {0x20, 0x10});
}

TEST_CASE("Serialize -123 to buffer")
{
    const auto value = FixPoint(int16_t(-123));
    expect_serialization_result(value, {0x27, 0xb0});
}

TEST_CASE("Serialize int16_t minimum value to buffer")
{
    const auto value = FixPoint(FixPoint::MIN_AS_INT16);
    expect_serialization_result(value, {0x3f, 0xf0});
}

TEST_CASE("Serialize 500.125 to buffer")
{
    const auto value = FixPoint(500.125);
    expect_serialization_result(value, {0x1f, 0x42});
}

TEST_CASE("Serialize maximum convertible double value to buffer")
{
    const auto value = FixPoint(FixPoint::MAX_AS_DOUBLE);
    expect_serialization_result(value, {0x1f, 0xff});
}

TEST_CASE("Serialize -88.875 to buffer")
{
    const auto value = FixPoint(-88.875);
    expect_serialization_result(value, {0x25, 0x8e});
}

TEST_CASE("Serialize minimum convertible double value to buffer")
{
    const auto value = FixPoint(FixPoint::MIN_AS_DOUBLE);
    expect_serialization_result(value, {0x3f, 0xff});
}

TEST_CASE("Serialize double NaN to buffer")
{
    const auto value = FixPoint(std::numeric_limits<double>::quiet_NaN());
    REQUIRE(value.is_nan());
    expect_serialization_result(value, {0x20, 0x00});
}

TEST_CASE("Deserialization from buffer requires minimum buffer size")
{
    const std::array<const uint8_t, 4> input{0x17, 0xfc, 0x00, 0x00};

    auto value = FixPoint(input.data(), 0);
    CHECK(value.is_nan());

    value = FixPoint(input.data(), 1);
    CHECK(value.is_nan());

    value = FixPoint(input.data(), 2);
    REQUIRE_FALSE(value.is_nan());
    expect_equal_doubles(value.to_double(), 383.75);

    value = FixPoint(input.data(), 3);
    REQUIRE_FALSE(value.is_nan());
    expect_equal_doubles(value.to_double(), 383.75);

    value = FixPoint(input.data(), 4);
    REQUIRE_FALSE(value.is_nan());
    expect_equal_doubles(value.to_double(), 383.75);
}

static void expect_deserialization_result(double expected,
                                          const std::array<const uint8_t, 2> &data)
{
    const auto value = FixPoint(data.data(), data.size());
    expect_equal_doubles(value.to_double(), expected);
}

TEST_CASE("Deserialize 0 from buffer")
{
    expect_deserialization_result(0.0, {0x00, 0x00});
}

TEST_CASE("Deserialize 0.5 from buffer")
{
    expect_deserialization_result(0.5, {0x00, 0x08});
}

TEST_CASE("Deserialize 1.5 from buffer")
{
    expect_deserialization_result(1.5, {0x00, 0x18});
}

TEST_CASE("Deserialize 491.8125 from buffer")
{
    expect_deserialization_result(491.8125, {0x1e, 0xbd});
}

TEST_CASE("Deserialize -0.5 from buffer")
{
    expect_deserialization_result(-0.5, {0x20, 0x08});
}

TEST_CASE("Deserialize -1.5 from buffer")
{
    expect_deserialization_result(-1.5, {0x20, 0x18});
}

TEST_CASE("Deserialize -367.3125 from buffer")
{
    expect_deserialization_result(-367.3125, {0x36, 0xf5});
}

TEST_CASE("Deserialize NaN from buffer")
{
    expect_deserialization_result(std::numeric_limits<double>::quiet_NaN(), {0x20, 0x00});
}

TEST_CASE("Deserialize maximum convertible double value from buffer")
{
    expect_deserialization_result(FixPoint::MAX_AS_DOUBLE, {0x1f, 0xff});
}

TEST_CASE("Deserialize minimum convertible double value from buffer")
{
    expect_deserialization_result(FixPoint::MIN_AS_DOUBLE, {0x3f, 0xff});
}

TEST_CASE("Deserialization considers 14 bits only")
{
    expect_deserialization_result( 0.0,  {0x80, 0x00});
    expect_deserialization_result( 0.0,  {0x40, 0x00});
    expect_deserialization_result( 0.0,  {0xc0, 0x00});
    expect_deserialization_result(-1.0,  {0xa0, 0x10});
    expect_deserialization_result( 5.5,  {0x40, 0x58});
    expect_deserialization_result(-8.25, {0xe0, 0x84});
    expect_deserialization_result(-0.75, {0xe0, 0x0c});
}

TEST_CASE("Rounding during conversion to native types")
{
    static const std::array<const std::tuple<const double, const int, const double>, 24> expectations
    {
        /* some simple cases */
        std::make_tuple( 5.4   ,  5,  5.375),
        std::make_tuple( 5.4999,  5,  5.5),
        std::make_tuple( 5.5   ,  6,  5.5),
        std::make_tuple( 5.5001,  6,  5.5),
        std::make_tuple( 5.6   ,  6,  5.625),
        std::make_tuple( 5.9999,  6,  6.0),
        std::make_tuple( 6.0001,  6,  6.0),
        std::make_tuple(-5.4   , -5, -5.375),
        std::make_tuple(-5.4999, -5, -5.5),
        std::make_tuple(-5.5   , -6, -5.5),
        std::make_tuple(-5.5001, -6, -5.5),
        std::make_tuple(-5.6   , -6, -5.625),
        std::make_tuple(-5.9999, -6, -6.0),
        std::make_tuple(-6.0001, -6, -6.0),
        std::make_tuple(double(FixPoint::MAX_AS_INT16) + 0.75,
                        FixPoint::MAX_AS_INT16 + 1,
                        double(FixPoint::MAX_AS_INT16) + 0.75),
        std::make_tuple(double(FixPoint::MIN_AS_INT16) - 0.75,
                        FixPoint::MIN_AS_INT16 - 1,
                        double(FixPoint::MIN_AS_INT16) - 0.75),

        /* cases found by randomized unit test */
        std::make_tuple(FixPoint::MIN_AS_DOUBLE - FixPoint::PRECISION / 2.0,
                        INT16_MIN,
                        std::numeric_limits<double>::quiet_NaN()),
        std::make_tuple(std::nextafter(FixPoint::MIN_AS_DOUBLE - FixPoint::PRECISION / 2.0, 0.0),
                        FixPoint::MIN_AS_INT16 - 1,
                        double(FixPoint::MIN_AS_DOUBLE)),
        std::make_tuple(FixPoint::MAX_AS_DOUBLE + FixPoint::PRECISION / 2.0,
                        INT16_MIN,
                        std::numeric_limits<double>::quiet_NaN()),
        std::make_tuple(std::nextafter(FixPoint::MAX_AS_DOUBLE + FixPoint::PRECISION / 2.0, 0.0),
                        FixPoint::MAX_AS_INT16 + 1,
                        double(FixPoint::MAX_AS_DOUBLE)),
        std::make_tuple( 3.4753,    3,  3.5),
        std::make_tuple(-3.4753,   -3, -3.5),
        std::make_tuple( 0.817499,  1,  0.8125),
        std::make_tuple(-0.817499, -1, -0.8125),
    };

    for(const auto &p : expectations)
    {
        const auto value = FixPoint(std::get<0>(p));
        const auto int_result = value.to_int16();
        const auto dbl_result = value.to_double();

        std::ostringstream os;
        os << "Failed for input " << std::setprecision(20) << std::get<0>(p);
        const auto s(os.str());

        CHECK_MESSAGE(int_result == std::get<1>(p), s);
        expect_equal_doubles(dbl_result, std::get<0>(p), std::get<2>(p));
    }
}

static int16_t compute_expected_int16(const double dbl)
{
    return ((dbl < FixPoint::MIN_AS_DOUBLE - (FixPoint::PRECISION / 2.0)) ||
            (dbl > FixPoint::MAX_AS_DOUBLE + (FixPoint::PRECISION / 2.0)))
        ? INT16_MIN
        : int16_t(std::round(dbl));
}

static double compute_expected_double(const double dbl)
{
    if((dbl <= FixPoint::MIN_AS_DOUBLE - (FixPoint::PRECISION / 2.0)) ||
       (dbl >= FixPoint::MAX_AS_DOUBLE + (FixPoint::PRECISION / 2.0)))
        return std::numeric_limits<double>::quiet_NaN();

    /* deliberately primitive algorithm to make it easy to analyze and to make
     * it different from the real implementation */
    const double abs_dbl = std::abs(dbl);
    const double pre = std::trunc(abs_dbl);
    double below = pre;
    double above = pre + 1.0;

    for(uint8_t i = 0; i <= FixPoint::MAX_DECIMAL_VALUE; ++i)
    {
        const double temp = pre + FixPoint::PRECISION * i;

        if(temp < abs_dbl)
            below = temp;
        else if(temp > abs_dbl)
        {
            above = temp;
            break;
        }
        else
            return dbl;
    }

    return std::copysign(abs_dbl - below <= above - abs_dbl ? below : above,
                         dbl);
}

static void check_computed_expected_double(const double expected, const double input)
{
    expect_equal_doubles(compute_expected_double(input), input, expected);
}

TEST_CASE("Assert integrity of expectation computation")
{
    static constexpr double lower_int16_boundary =
        double(FixPoint::MIN_AS_INT16 - 1) + FixPoint::PRECISION / 2.0;
    static constexpr double upper_int16_boundary =
        double(FixPoint::MAX_AS_INT16 + 1) - FixPoint::PRECISION / 2.0;

    /* computations for integers */
    CHECK(compute_expected_int16(FixPoint::MIN_AS_INT16) == FixPoint::MIN_AS_INT16);
    CHECK(compute_expected_int16(FixPoint::MAX_AS_INT16) == FixPoint::MAX_AS_INT16);
    CHECK(compute_expected_int16(FixPoint::MIN_AS_INT16 - 1) == INT16_MIN);
    CHECK(compute_expected_int16(FixPoint::MAX_AS_INT16 + 1) == INT16_MIN);

    CHECK(compute_expected_int16(double(FixPoint::MIN_AS_INT16)) == FixPoint::MIN_AS_INT16);
    CHECK(compute_expected_int16(double(FixPoint::MAX_AS_INT16)) == FixPoint::MAX_AS_INT16);
    CHECK(compute_expected_int16(double(FixPoint::MIN_AS_INT16 - 1)) == INT16_MIN);
    CHECK(compute_expected_int16(double(FixPoint::MAX_AS_INT16 + 1)) == INT16_MIN);

    CHECK(compute_expected_int16(lower_int16_boundary) == int16_t(FixPoint::MIN_AS_INT16 - 1));
    CHECK(compute_expected_int16(std::nextafter(lower_int16_boundary,
                                                std::numeric_limits<double>::lowest())) == INT16_MIN);
    CHECK(compute_expected_int16(upper_int16_boundary) == int16_t(FixPoint::MAX_AS_INT16 + 1));
    CHECK(compute_expected_int16(std::nextafter(upper_int16_boundary,
                                                std::numeric_limits<double>::max())) == INT16_MIN);

    /* computations for doubles */
    static const double lower_double_boundary =
        FixPoint::MIN_AS_DOUBLE - FixPoint::PRECISION / 2.0;
    static const double upper_double_boundary =
        FixPoint::MAX_AS_DOUBLE + FixPoint::PRECISION / 2.0;

    expect_equal_doubles(lower_int16_boundary, lower_double_boundary);
    expect_equal_doubles(upper_int16_boundary, upper_double_boundary);

    check_computed_expected_double(FixPoint::MIN_AS_INT16, FixPoint::MIN_AS_INT16);
    check_computed_expected_double(FixPoint::MAX_AS_INT16, FixPoint::MAX_AS_INT16);
    check_computed_expected_double(std::numeric_limits<double>::quiet_NaN(),
                                   FixPoint::MIN_AS_INT16 - 1);
    check_computed_expected_double(std::numeric_limits<double>::quiet_NaN(),
                                   FixPoint::MAX_AS_INT16 + 1);

    check_computed_expected_double(double(FixPoint::MIN_AS_INT16 - 1) + FixPoint::PRECISION,
                                   std::nextafter(lower_double_boundary, 0.0));
    check_computed_expected_double(std::numeric_limits<double>::quiet_NaN(),
                                   lower_double_boundary);

    check_computed_expected_double(double(FixPoint::MAX_AS_INT16 + 1) - FixPoint::PRECISION,
                                   std::nextafter(upper_double_boundary, 0.0));
    check_computed_expected_double(std::numeric_limits<double>::quiet_NaN(),
                                   upper_double_boundary);
}

static void check_expected_conversion_to_native_types(double dbl)
{
    const auto value = FixPoint(dbl);
    const auto int_result = value.to_int16();
    const auto dbl_result = value.to_double();

    const int16_t expected_int16 = compute_expected_int16(dbl);
    const double expected_double = compute_expected_double(dbl);

    std::ostringstream os;
    os << "Conversion failed for input " << std::setprecision(20) << dbl;
    const auto s(os.str());

    CHECK_MESSAGE(int_result == expected_int16, s);
    expect_equal_doubles(dbl_result, dbl, expected_double);
}

static void check_back_and_forth_conversions_are_stable(double dbl)
{
    const auto value = FixPoint(dbl);
    const auto dbl_result = value.to_double();
    const auto dbl_again = FixPoint(dbl_result).to_double();;

    expect_equal_doubles(dbl_again, dbl, dbl_result);

    const auto int_result = value.to_int16();
    const auto int_again = FixPoint(int_result).to_int16();

    std::ostringstream os;
    os << "Multiple conversions failed for input " << std::setprecision(20) << dbl;
    const auto failmsg(os.str());

    if(value.is_nan())
    {
        CHECK_MESSAGE(int_result == INT16_MIN, failmsg);
        CHECK_MESSAGE(int_again  == INT16_MIN, failmsg);
    }
    else
    {
        if(int_result >= FixPoint::MIN_AS_INT16 && int_result <= FixPoint::MAX_AS_INT16)
            CHECK_MESSAGE(int_again  == int_result, failmsg);
        else if(int_result == FixPoint::MIN_AS_INT16 - 1 ||
                int_result == FixPoint::MAX_AS_INT16 + 1)
            CHECK_MESSAGE(int_again  == INT16_MIN, failmsg);
        else
            FAIL_CHECK(failmsg);
    }
}

using knuth_lcg =
    std::linear_congruential_engine<uint64_t, 6364136223846793005U, 1442695040888963407U, 0U>;

static void random_tests_with_distribution(unsigned int count, knuth_lcg &prng,
                                           std::uniform_real_distribution<double> &&pdist)
{
    for(unsigned int i = 0; i < count; ++i)
    {
        const double dbl = pdist(prng);

        check_expected_conversion_to_native_types(dbl);
        check_back_and_forth_conversions_are_stable(dbl);
    }
}

TEST_CASE("Conversion of random inputs for rounding")
{
    std::random_device rd;
    knuth_lcg prng(uint64_t(rd()) << 32 | rd());

    random_tests_with_distribution(30000, prng,
        std::move(std::uniform_real_distribution<double>(double(FixPoint::MIN_AS_INT16) - 2,
                                                         double(FixPoint::MAX_AS_INT16) + 2)));

    random_tests_with_distribution(5000, prng,
        std::move(std::uniform_real_distribution<double>(double(FixPoint::MIN_AS_INT16) - 5,
                                                         double(FixPoint::MIN_AS_INT16) + 5)));

    random_tests_with_distribution(5000, prng,
        std::move(std::uniform_real_distribution<double>(double(FixPoint::MAX_AS_INT16) - 5,
                                                         double(FixPoint::MAX_AS_INT16) + 5)));
}

TEST_SUITE_END();
