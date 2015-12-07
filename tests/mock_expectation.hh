/*
 * Copyright (C) 2015, 2019  T+A elektroakustik GmbH & Co. KG
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

#ifndef MOCK_EXPECTATION_HH
#define MOCK_EXPECTATION_HH

#include <cppcutter.h>
#include <vector>

template <typename E>
class MockExpectationsTemplate
{
  private:

    std::vector<E> expectations_;
    size_t next_checked_expectation_;

  public:
    MockExpectationsTemplate(const MockExpectationsTemplate &) = delete;
    MockExpectationsTemplate(MockExpectationsTemplate &&) = default;
    MockExpectationsTemplate &operator=(const MockExpectationsTemplate &) = delete;

    explicit MockExpectationsTemplate() {}

    void init()
    {
        expectations_.clear();
        next_checked_expectation_ = 0;
    }

    void check() const
    {
        cppcut_assert_equal(next_checked_expectation_, expectations_.size(),
                            cut_message("In %s:\nHave %zu expectation%s, but only %zu %s checked",
                                        __PRETTY_FUNCTION__,
                                        expectations_.size(),
                                        (expectations_.size() == 1) ? "" : "s",
                                        next_checked_expectation_,
                                        (next_checked_expectation_ == 1) ? "was" : "were"));
    }

    void add(E &&expectation)
    {
        expectations_.emplace_back(std::move(expectation));
    }

    const E &get_next_expectation(const char *string)
    {
        cppcut_assert_operator(next_checked_expectation_, <, expectations_.size(),
                               cut_message("Missing expectation for \"%s\"",
                                           string));

        return expectations_[next_checked_expectation_++];
    }

    const E &get_next_expectation(const char *format_string, va_list va)
    {
        va_list copy;
        va_copy(copy, va);

        va_copy(copy, va);
        int len = vsnprintf(NULL, 0, format_string, copy);
        char buffer[len + 1];

        va_copy(copy, va);
        vsnprintf(buffer, sizeof(buffer), format_string, copy);

        return get_next_expectation(buffer);
    }
};

#endif /* !MOCK_EXPECTATION_HH */
