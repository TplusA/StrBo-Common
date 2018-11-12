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

#ifndef MOCK_EXPECTATION_HH
#define MOCK_EXPECTATION_HH

#include <doctest.h>
#include <string>
#include <list>
#include <memory>
#include <cstdio>
#include <cstdarg>

template <typename E>
class MockExpectationsTemplate
{
  private:
    const std::string mock_id_;

    std::list<std::unique_ptr<E>> expectations_;
    typename decltype(expectations_)::const_iterator next_checked_expectation_;
    bool next_checked_expectation_needs_initialization_;
    bool next_checked_is_front_;

  public:
    MockExpectationsTemplate(const MockExpectationsTemplate &) = delete;
    MockExpectationsTemplate(MockExpectationsTemplate &&) = default;
    MockExpectationsTemplate &operator=(const MockExpectationsTemplate &) = delete;
    MockExpectationsTemplate &operator=(MockExpectationsTemplate &&) = default;

    explicit MockExpectationsTemplate(std::string &&mock_id):
        mock_id_(mock_id),
        next_checked_expectation_(expectations_.begin()),
        next_checked_expectation_needs_initialization_(true),
        next_checked_is_front_(true)
    {}

    void done() const
    {
        if(next_checked_expectation_ == expectations_.end())
            return;

        const auto total_count = expectations_.size();
        const auto checked_count =
            std::distance(expectations_.begin(), next_checked_expectation_);
        const char *plural_s = (expectations_.size() == 1 ? "" : "s");
        const char *was_were = (checked_count == 1 ? "was" : "were");

        MESSAGE(mock_id_ << ": Having " << total_count <<
                " expectation" << plural_s << ", but only " <<
                checked_count << " " << was_were << " checked");
        FAIL("Too many expectations for " << mock_id_);
    }

    void add(std::unique_ptr<E> expectation)
    {
        expectations_.emplace_back(std::move(expectation));

        if(next_checked_expectation_needs_initialization_)
        {
            if(next_checked_is_front_)
            {
                next_checked_expectation_ = expectations_.begin();
                next_checked_is_front_ = false;
            }
            else
                next_checked_expectation_ = --expectations_.end();

            next_checked_expectation_needs_initialization_ = false;
        }
    }

    template <typename T, typename ... Args>
    typename T::CheckReturnType check_and_advance(Args ... args)
    {
        if(next_checked_expectation_ == expectations_.end())
        {
            const auto n = expectations_.size();

            MESSAGE(mock_id_ << ": Code under test does more than it was expected to do. "
                    "Please fix the code, or expect() more from it (fix the test).");

            if(n == 1)
                MESSAGE(mock_id_ << ": There is only 1 expectation defined.");
            else
                MESSAGE("There are only " << n << " defined.");

            FAIL("Missing expectations for " << mock_id_);
        }

        const auto *ptr = dynamic_cast<const T *>(next_checked_expectation_->get());
        REQUIRE_MESSAGE(ptr != nullptr, mock_id_ << ": Expectation type mismatch");

        ++next_checked_expectation_;

        if(next_checked_expectation_ == expectations_.end())
            next_checked_expectation_needs_initialization_ = true;

        return ptr->check(args...);
    }

    template <typename T>
    const T &next(const char *string)
    {
        REQUIRE_MESSAGE(next_checked_expectation_ != expectations_.end(),
                        "Missing expectation for \"" << string << "\"");
        REQUIRE_MESSAGE(dynamic_cast<const T *>(next_checked_expectation_->get()) != nullptr,
                        "Expectation type mismatch for \"" << string << "\"");
        return *static_cast<const T *>((next_checked_expectation_++)->get());
    }

    template <typename T>
    const T &next(const char *format_string, va_list va)
    {
        va_list copy;
        va_copy(copy, va);

        va_copy(copy, va);
        auto len = std::vsnprintf(nullptr, 0, format_string, copy);
        char buffer[len + 1];

        va_copy(copy, va);
        std::vsnprintf(buffer, sizeof(buffer), format_string, copy);

        return next<T>(buffer);
    }
};

#endif /* !MOCK_EXPECTATION_HH */