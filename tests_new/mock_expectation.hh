/*
 * Copyright (C) 2018--2022  T+A elektroakustik GmbH & Co. KG
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

#include <doctest.h>
#include <string>
#include <list>
#include <memory>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <utility>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>

template <typename E>
class MockExpectationsTemplate
{
  private:
    const std::string mock_id_;

    std::list<std::unique_ptr<E>> expectations_;
    typename decltype(expectations_)::const_iterator next_checked_expectation_;
    bool next_checked_expectation_needs_initialization_;
    bool next_checked_is_front_;

    std::unordered_map<std::type_index, std::unique_ptr<E>> ignored_;

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
        FAIL_CHECK("Too many expectations for " << mock_id_);
    }

    E *add(std::unique_ptr<E> expectation)
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

        return expectations_.rbegin()->get();
    }

    template <typename T>
    void ignore(std::unique_ptr<E> default_result)
    {
        ignored_[std::type_index(typeid(T))] = std::move(default_result);
    }

    template <typename T>
    void allow()
    {
        ignored_.erase(std::type_index(typeid(T)));
    }

  private:
    struct expectation_default_ignore {};
    struct expectation_ignore_behavior: expectation_default_ignore {};
    template<typename> struct expectation_ignores_by_fn { using type = bool; };
    template<typename> struct expectation_ignores_by_value { using type = bool; };

    template <typename T, typename R,
              typename std::enable_if<!std::is_same<R, void>::value, bool>::type = 0,
              typename expectation_ignores_by_fn<decltype(&T::ignored)>::type = 0>
    R ignore_expectation(const T *default_result, expectation_ignore_behavior)
    {
        return default_result->ignored();
    }

    template <typename T, typename R,
              typename std::enable_if<!std::is_same<R, void>::value, bool>::type = 0,
              typename expectation_ignores_by_value<decltype(T::retval_)>::type = 0>
    R ignore_expectation(const T *default_result, expectation_ignore_behavior)
    {
        return default_result->retval_;
    }

    template <typename T, typename R>
    R ignore_expectation(const T *, expectation_default_ignore)
    {
        return R();
    }

  public:
    template <typename T, typename R, typename ... Args>
    R check_and_advance(Args ... args)
    {
        const auto &ignored(ignored_.find(std::type_index(typeid(T))));

        if(ignored != ignored_.end())
            return ignore_expectation<T, R>(dynamic_cast<const T *>(ignored->second.get()),
                                            expectation_ignore_behavior());

        if(next_checked_expectation_ == expectations_.end())
        {
            const auto n = expectations_.size();

            MESSAGE(mock_id_ << ": Code under test does more than it was expected to do. "
                    "Please fix the code, or expect() more from it (fix the test).");

            if(n == 0)
                MESSAGE(mock_id_ << ": There are no expectations defined.");
            else if(n == 1)
                MESSAGE(mock_id_ << ": There is only 1 expectation defined.");
            else
                MESSAGE("There are only " << n << " expectations defined.");

            FAIL("Missing expectations for " << mock_id_ << ": " << typeid(T).name());
        }

        auto *ptr = dynamic_cast<T *>(next_checked_expectation_->get());

        if(ptr == nullptr)
        {
            const auto &name(typeid(T).name());
            REQUIRE_MESSAGE(ptr != nullptr,
                            mock_id_ << ": Expectation type mismatch, expected " << name);
        }

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
        std::vector<char> buffer;
        buffer.resize(len + 1);

        va_copy(copy, va);
        std::vsnprintf(buffer.data(), buffer.size(), format_string, copy);

        return next<T>(buffer.data());
    }
};

#if __cplusplus >= 201402L
/*!
 * Function template for adding expectations to mocks.
 *
 * We have put it in here at global scope so that mock objects don't need to
 * implement their own versions. Mocks must, however, provide an expect()
 * member function which accepts a std::unique_ptr<T>, where T is the type of
 * the expectation to be added.
 */
template <typename T, typename MT, typename ... Args>
static inline void expect(MT &mock, Args ... args)
{
    mock->expect(std::make_unique<T>(std::forward<Args>(args)...));
}
#endif /* C++14 */

#endif /* !MOCK_EXPECTATION_HH */
