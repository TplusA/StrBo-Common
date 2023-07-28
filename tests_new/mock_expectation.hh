/*
 * Copyright (C) 2018--2023  T+A elektroakustik GmbH & Co. KG
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
#include <limits>

class MockExpectationSequence
{
  private:
    const std::string eseq_id_;
    unsigned int next_assigned_serial_;
    unsigned int next_checked_serial_;
    std::list<std::tuple<std::string, std::string>> names_;

  public:
    MockExpectationSequence(const MockExpectationSequence &) = delete;
    MockExpectationSequence(MockExpectationSequence &&) = default;
    MockExpectationSequence &operator=(const MockExpectationSequence &) = delete;
    MockExpectationSequence &operator=(MockExpectationSequence &&) = delete;

    explicit MockExpectationSequence(std::string &&eseq_id = "Expectation Sequence"):
        eseq_id_(std::move(eseq_id)),
        next_assigned_serial_(0),
        next_checked_serial_(0)
    {}

    void reset()
    {
        next_assigned_serial_ = 0;
        next_checked_serial_ = 0;
        names_.clear();
    }

    void done()
    {
        CHECK(next_assigned_serial_ == names_.size());

        if(next_checked_serial_ == names_.size())
        {
            reset();
            return;
        }

        const std::string plural_s = (next_assigned_serial_ == 1 ? "" : "s");
        const std::string was_were = (next_checked_serial_ == 1 ? "was" : "were");

        MESSAGE(eseq_id_ << ": Having sequence of " << next_assigned_serial_ <<
                " expectation" << plural_s << ", but only " <<
                next_checked_serial_ << " " << was_were << " checked");
        show_missing_expectations(next_assigned_serial_);
        FAIL_CHECK("Too many expectations in " << eseq_id_);

        reset();
    }

    unsigned int make_serial(std::string &&name, std::string &&details)
    {
        names_.emplace_back(std::move(name), std::move(details));
        return next_assigned_serial_++;
    }

    void check(unsigned int serial, const std::string &name)
    {
        if(serial == next_checked_serial_)
        {
            ++next_checked_serial_;
            return;
        }

        if(next_checked_serial_ < names_.size())
        {
            FAIL_CHECK(eseq_id_ << ": observed unexpected " << name <<
                       " with serial " << serial << ", but expected serial " <<
                       next_checked_serial_);
            show_missing_expectations(serial);
        }
        else
            FAIL_CHECK(
                eseq_id_ << ": expected no more expectations, but observed " <<
                name << " with serial " << serial);

        CHECK(serial < names_.size());
        FAIL(eseq_id_ << ": out of sync");
    }

    void dump_last_checked() const
    {
        if(next_assigned_serial_ == 0 || next_checked_serial_ == 0)
            return;

        static constexpr unsigned int DEPTH = 5;

        const unsigned int first =
            next_checked_serial_ > DEPTH ? next_checked_serial_ - DEPTH : 0;

        auto it(std::next(names_.begin(), first));
        for(unsigned int i = first; i < next_checked_serial_; ++i, ++it)
        {
            const auto &details(std::get<1>(*it));
            if(details.empty())
                MESSAGE("Already checked serial " << i << ": " << std::get<0>(*it));
            else
                MESSAGE("Already checked serial " << i << " -- " << std::get<0>(*it)
                        << ": " << details);
        }
    }

  private:
    void show_missing_expectations(unsigned int up_to_serial) const
    {
        REQUIRE(next_checked_serial_ < up_to_serial);

        auto it(std::next(names_.begin(), next_checked_serial_));
        for(unsigned int i = next_checked_serial_; i < up_to_serial; ++i, ++it)
        {
            const auto &details(std::get<1>(*it));
            if(details.empty())
                MESSAGE("Expected serial " << i << ": " << std::get<0>(*it));
            else
                MESSAGE("Expected serial " << i << ": " << std::get<0>(*it)
                        << ": " << details);
        }
    }
};

#ifdef MOCK_EXPECTATION_WITH_EXPECTATION_SEQUENCE_SINGLETON
extern std::shared_ptr<MockExpectationSequence> mock_expectation_sequence_singleton;
#endif /* MOCK_EXPECTATION_WITH_EXPECTATION_SEQUENCE_SINGLETON */

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

    std::shared_ptr<MockExpectationSequence> eseq_;

  public:
    MockExpectationsTemplate(const MockExpectationsTemplate &) = delete;
    MockExpectationsTemplate(MockExpectationsTemplate &&) = default;
    MockExpectationsTemplate &operator=(const MockExpectationsTemplate &) = delete;
    MockExpectationsTemplate &operator=(MockExpectationsTemplate &&) = default;

    explicit MockExpectationsTemplate(std::string &&mock_id,
                                      std::shared_ptr<MockExpectationSequence> eseq = nullptr):
        mock_id_(mock_id),
        next_checked_expectation_(expectations_.begin()),
        next_checked_expectation_needs_initialization_(true),
        next_checked_is_front_(true),
#ifdef MOCK_EXPECTATION_WITH_EXPECTATION_SEQUENCE_SINGLETON
        eseq_(eseq != nullptr ? std::move(eseq) : mock_expectation_sequence_singleton)
#else /* !MOCK_EXPECTATION_WITH_EXPECTATION_SEQUENCE_SINGLETON */
        eseq_(std::move(eseq))
#endif /* MOCK_EXPECTATION_WITH_EXPECTATION_SEQUENCE_SINGLETON */
    {}

    void done() const
    {
        if(next_checked_expectation_ == expectations_.end())
            return;

        const auto total_count = expectations_.size();
        const auto checked_count =
            std::distance(expectations_.begin(), next_checked_expectation_);
        const std::string plural_s = (expectations_.size() == 1 ? "" : "s");
        const std::string was_were = (checked_count == 1 ? "was" : "were");

        MESSAGE(mock_id_ << ": Having " << total_count <<
                " expectation" << plural_s << ", but only " <<
                checked_count << " " << was_were << " checked");
        FAIL_CHECK("Too many expectations for " << mock_id_);
    }

    E *add(std::unique_ptr<E> expectation)
    {
        if(eseq_ != nullptr)
            expectation->set_sequence_serial(eseq_->make_serial(
                            mock_id_ + "." + expectation->get_name(),
                            expectation->get_details()));

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

            if(eseq_ != nullptr)
                eseq_->dump_last_checked();

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

        if(eseq_ != nullptr)
            eseq_->check(ptr->get_sequence_serial(),
                         mock_id_ + "." + ptr->get_name());

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
