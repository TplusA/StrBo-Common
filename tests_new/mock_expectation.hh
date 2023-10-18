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
#include <functional>

class MockExpectationBase
{
  private:
    std::string name_;
    unsigned int sequence_serial_;

  protected:
    MockExpectationBase(std::string &&name):
        name_(std::move(name)),
        sequence_serial_(std::numeric_limits<unsigned int>::max())
    {{}}

  public:
    MockExpectationBase(const MockExpectationBase &) = delete;
    MockExpectationBase(MockExpectationBase &&) = default;
    MockExpectationBase &operator=(const MockExpectationBase &) = delete;
    MockExpectationBase &operator=(MockExpectationBase &&) = default;
    virtual ~MockExpectationBase() = default;

    const std::string &get_name() const { return name_; }
    void set_sequence_serial(unsigned int ss) { sequence_serial_ = ss; }
    unsigned int get_sequence_serial() const { return sequence_serial_; }

    virtual std::string get_details() const { return ""; }
};

class MockExpectationSequence
{
  private:
    const std::string eseq_id_;
    unsigned int next_assigned_serial_;
    unsigned int next_checked_serial_;
    std::list<std::pair<MockExpectationBase *, std::string>> e_;

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
        e_.clear();
    }

    void done()
    {
        CHECK(next_assigned_serial_ == e_.size());

        if(next_checked_serial_ == e_.size())
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

    void register_expectation(MockExpectationBase *expectation, std::string &&name)
    {
        expectation->set_sequence_serial(next_assigned_serial_++);
        e_.emplace_back(expectation, std::move(name));
    }

    bool check(unsigned int serial, const std::string &name)
    {
        if(serial == next_checked_serial_)
        {
            ++next_checked_serial_;
            return true;
        }

        if(next_checked_serial_ < e_.size())
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

        CHECK(serial < e_.size());
        FAIL_CHECK(eseq_id_ << ": out of sync");
        return false;
    }

    void dump_last_checked() const
    {
        if(next_assigned_serial_ == 0 || next_checked_serial_ == 0)
            return;

        static constexpr unsigned int DEPTH = 5;

        const unsigned int first =
            next_checked_serial_ > DEPTH ? next_checked_serial_ - DEPTH : 0;

        auto it(std::next(e_.begin(), first));
        for(unsigned int i = first; i < next_checked_serial_; ++i, ++it)
        {
            const auto &details(it->first->get_details());
            if(details.empty())
                MESSAGE("Already checked serial " << i << ": " << it->second);
            else
                MESSAGE("Already checked serial " << i << " -- " << it->second
                        << ": " << details);
        }
    }

    void move_range(int src, unsigned int count, int dest, bool verbose = false)
    {
        verbose_move_begin(verbose, "MOVE");
        REQUIRE(count > 0);

        const auto [srcidx, destidx] = compute_move_indices(src, dest);

        if(srcidx == destidx)
        {
            verbose_move_end(verbose, "MOVE");
            return;
        }

        if(srcidx < destidx)
            REQUIRE(destidx + count <= next_assigned_serial_);
        else
            REQUIRE(srcidx + count <= next_assigned_serial_);

        if(verbose)
            MESSAGE("Move " << count << " indices from " << srcidx << " to " << destidx);

        // TODO: We can rewrite indices and count so that we always move from
        //       right to left, which is more efficient.
        const bool left_to_right = srcidx < destidx;

        decltype(e_)::iterator it_src;
        decltype(e_)::iterator it_dest;

        if(left_to_right)
            it_src = std::next(e_.begin(), srcidx);
        else
        {
            it_dest = std::next(e_.begin(), destidx);
            it_src = std::next(it_dest, srcidx - destidx);
        }

        decltype(e_) temp;
        temp.splice(temp.begin(), e_, it_src, std::next(it_src, count));

        if(left_to_right)
        {
            MESSAGE("Please rewrite move_range() parameters so that we are "
                    "moving from right to left (which is more efficient), "
                    "or improve this implementation to do this automatically");
            it_src = std::next(e_.begin(), srcidx);
            it_dest = std::next(it_src, destidx - srcidx);
        }

        e_.splice(it_dest, std::move(temp));

        /* re-assign serials to whole affected range */
        const auto left = std::min(srcidx, destidx);
        const auto right = std::max(srcidx, destidx) + count;
        auto it(std::next(e_.begin(), left));
        const auto end(std::next(it, right - left));
        unsigned int serial = left;

        while(it != end)
        {
            it->first->set_sequence_serial(serial++);
            ++it;
        }

        verbose_move_end(verbose, "MOVE");
    }

    void swap_slots(int a, int b, bool verbose = false)
    {
        verbose_move_begin(verbose, "SWAP");

        auto [aidx, bidx] = compute_move_indices(a, b);

        if(aidx == bidx)
        {
            verbose_move_end(verbose, "SWAP");
            return;
        }

        if(aidx > bidx)
            std::swap(aidx, bidx);

        if(verbose)
            MESSAGE("Swap indices " << aidx << " and " << bidx);

        auto it1 = std::next(e_.begin(), aidx);
        auto it2 = std::next(it1, bidx - aidx);

        it1->first->set_sequence_serial(bidx);
        it2->first->set_sequence_serial(aidx);
        std::iter_swap(it1, it2);

        verbose_move_end(verbose, "SWAP");
    }

    void reverse_last_slots(bool verbose = false)
    {
        swap_slots(-1, -2, verbose);
    }

  private:
    void verbose_move_begin(bool verbose, const std::string &which) const
    {
        if(!verbose)
            return;

        MESSAGE(">>>>>  " << which << " BEGIN  <<<<<");
        dump_last_checked();
        show_missing_expectations(next_assigned_serial_);
        MESSAGE(">>>>>  ----------  <<<<<");
    }

    void verbose_move_end(bool verbose, const std::string &which) const
    {
        if(!verbose)
            return;

        dump_last_checked();
        show_missing_expectations(next_assigned_serial_);
        MESSAGE(">>>>>   " << which << " END   <<<<<");
    }

    std::pair<unsigned int, unsigned int> compute_move_indices(int a, int b) const
    {
        const unsigned int apos = a >= 0 ? a : -(a + 1);
        const unsigned int bpos = b >= 0 ? b : -(b + 1);

        REQUIRE(apos < next_assigned_serial_);
        REQUIRE(bpos < next_assigned_serial_);

        const auto aidx = a >= 0 ? a : next_assigned_serial_ + a;
        const auto bidx = b >= 0 ? b : next_assigned_serial_ + b;

        REQUIRE(aidx >= next_checked_serial_);
        REQUIRE(bidx >= next_checked_serial_);

        return {aidx, bidx};
    }

    void show_missing_expectations(unsigned int up_to_serial) const
    {
        REQUIRE(next_checked_serial_ < up_to_serial);
        REQUIRE(up_to_serial <= next_assigned_serial_);

        auto it(std::next(e_.begin(), next_checked_serial_));
        for(unsigned int i = next_checked_serial_; i < up_to_serial; ++i, ++it)
        {
            const auto &details(it->first->get_details());
            if(details.empty())
                MESSAGE("Expected serial " << i << ": " << it->second);
            else
                MESSAGE("Expected serial " << i << ": " << it->second
                        << ": " << details);
        }
    }
};

#ifdef MOCK_EXPECTATION_WITH_EXPECTATION_SEQUENCE_SINGLETON
extern std::shared_ptr<MockExpectationSequence> mock_expectation_sequence_singleton;
#endif /* MOCK_EXPECTATION_WITH_EXPECTATION_SEQUENCE_SINGLETON */

class MockBase
{
  private:
    const std::string mock_id_;

    std::list<std::unique_ptr<MockExpectationBase>> expectations_;
    std::list<std::unique_ptr<MockExpectationBase>>::const_iterator next_checked_expectation_;

    bool next_checked_expectation_needs_initialization_;
    bool next_checked_is_front_;

    std::unordered_map<std::type_index, std::unique_ptr<MockExpectationBase>> ignored_;
    bool ignore_all_;

    std::shared_ptr<MockExpectationSequence> eseq_;

  protected:
    explicit MockBase(std::string &&mock_id,
                      std::shared_ptr<MockExpectationSequence> eseq = nullptr):
        mock_id_(std::move(mock_id)),
        next_checked_expectation_(expectations_.begin()),
        next_checked_expectation_needs_initialization_(true),
        next_checked_is_front_(true),
        ignore_all_(false),
#ifdef MOCK_EXPECTATION_WITH_EXPECTATION_SEQUENCE_SINGLETON
        eseq_(eseq != nullptr ? std::move(eseq) : mock_expectation_sequence_singleton)
#else /* !MOCK_EXPECTATION_WITH_EXPECTATION_SEQUENCE_SINGLETON */
        eseq_(std::move(eseq))
#endif /* MOCK_EXPECTATION_WITH_EXPECTATION_SEQUENCE_SINGLETON */
    {}

  public:
    MockBase(const MockBase &) = delete;
    MockBase &operator=(const MockBase &) = delete;
    virtual ~MockBase() = default;

    void sort_expectations_by_sequence_id()
    {
        const auto pos = std::distance(expectations_.cbegin(), next_checked_expectation_);
        expectations_.sort([] (const auto &a, const auto &b)
                           { return a->get_sequence_serial() < b->get_sequence_serial(); });
        next_checked_expectation_ = std::next(expectations_.cbegin(), pos);
    }

    void done() const
    {
        if(next_checked_expectation_ == expectations_.end())
            return;

        const auto total_count = expectations_.size();
        const auto checked_count =
            std::distance(expectations_.cbegin(), next_checked_expectation_);
        const std::string plural_s = (expectations_.size() == 1 ? "" : "s");
        const std::string was_were = (checked_count == 1 ? "was" : "were");

        MESSAGE(mock_id_ << ": Having " << total_count <<
                " expectation" << plural_s << ", but only " <<
                checked_count << " " << was_were << " checked");
        FAIL_CHECK("Too many expectations for " << mock_id_);
    }

  protected:
    MockExpectationBase *add(std::unique_ptr<MockExpectationBase> expectation)
    {
        if(eseq_ != nullptr)
            eseq_->register_expectation(expectation.get(),
                                        mock_id_ + "." + expectation->get_name());

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
    void ignore(std::unique_ptr<MockExpectationBase> default_result)
    {
        static_assert(std::is_base_of_v<MockExpectationBase, T> == true);
        ignored_[std::type_index(typeid(T))] = std::move(default_result);
    }

  public:
    void ignore_all(bool ignore_mode = true)
    {
        ignore_all_ = ignore_mode;
    }

    template <typename T>
    void allow()
    {
        static_assert(std::is_base_of_v<MockExpectationBase, T> == true);
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
        static_assert(std::is_base_of_v<MockExpectationBase, T> == true);
        return default_result->ignored();
    }

    template <typename T, typename R,
              typename std::enable_if<!std::is_same<R, void>::value, bool>::type = 0,
              typename expectation_ignores_by_value<decltype(T::retval_)>::type = 0>
    R ignore_expectation(const T *default_result, expectation_ignore_behavior)
    {
        static_assert(std::is_base_of_v<MockExpectationBase, T> == true);
        return default_result->retval_;
    }

    template <typename T, typename R>
    R ignore_expectation(const T *, expectation_default_ignore)
    {
        static_assert(std::is_base_of_v<MockExpectationBase, T> == true);
        return R();
    }

  public:
    template <typename T, typename ... Args>
    auto check_next(Args ... args) -> decltype(std::declval<T>().check(args...))
    {
        static_assert(std::is_base_of_v<MockExpectationBase, T> == true);
        using R = decltype(std::declval<T>().check(args...));

        const auto &ignored(ignored_.find(std::type_index(typeid(T))));

        if(ignore_all_)
            return ignore_expectation<T, R>(static_cast<const T *>(nullptr),
                                            expectation_default_ignore());

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

            MESSAGE("Missing: " << T::make_from_check_parameters(args...)->get_details());
            FAIL("Missing expectations for " << mock_id_ << ": " << std::string(typeid(T).name()));
        }

        auto *ptr = dynamic_cast<T *>(next_checked_expectation_->get());

        if(ptr == nullptr)
        {
            const std::string &name(typeid(T).name());
            REQUIRE_MESSAGE(ptr != nullptr,
                            mock_id_ << ": Expectation type mismatch, expected " << name);
        }

        ++next_checked_expectation_;

        if(next_checked_expectation_ == expectations_.end())
            next_checked_expectation_needs_initialization_ = true;

        if(eseq_ != nullptr && !eseq_->check(ptr->get_sequence_serial(),
                                             mock_id_ + "." + ptr->get_name()))
            FAIL("Not expected now: ", ptr->get_details());

        return ptr->check(args...);
    }

    template <typename T>
    const T &next(const char *string)
    {
        static_assert(std::is_base_of_v<MockExpectationBase, T> == true);
        REQUIRE_MESSAGE(next_checked_expectation_ != expectations_.end(),
                        "Missing expectation for \"" << string << "\"");
        REQUIRE_MESSAGE(dynamic_cast<const T *>(next_checked_expectation_->get()) != nullptr,
                        "Expectation type mismatch for \"" << string << "\"");
        return *static_cast<const T *>((next_checked_expectation_++)->get());
    }

    template <typename T>
    const T &next(const char *format_string, va_list va)
    {
        static_assert(std::is_base_of_v<MockExpectationBase, T> == true);
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
    static_assert(std::is_base_of_v<MockExpectationBase, T> == true);
    static_assert(std::is_base_of_v<MockBase, MT> == true);
    mock->expect(std::make_unique<T>(std::forward<Args>(args)...));
}

template <typename T, typename MT, typename ... Args>
static inline void expect(std::unique_ptr<MT> &mock, Args ... args)
{
    static_assert(std::is_base_of_v<MockExpectationBase, T> == true);
    static_assert(std::is_base_of_v<MockBase, MT> == true);
    mock->expect(std::make_unique<T>(std::forward<Args>(args)...));
}

template <typename T, typename MT, typename ... Args>
static inline void expect(std::shared_ptr<MT> &mock, Args ... args)
{
    static_assert(std::is_base_of_v<MockExpectationBase, T> == true);
    static_assert(std::is_base_of_v<MockBase, MT> == true);
    mock->expect(std::make_unique<T>(std::forward<Args>(args)...));
}

#endif /* !MOCK_EXPECTATION_HH */
