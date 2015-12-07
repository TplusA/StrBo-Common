/*
 * Copyright (C) 2020  T+A elektroakustik GmbH & Co. KG
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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <cppcutter.h>
#include <string>

#include "mock_backtrace.hh"

enum class BacktraceFn
{
    log,
    dump,

    first_valid_backtrace_fn_id = log,
    last_valid_backtrace_fn_id = dump,
};

static std::ostream &operator<<(std::ostream &os, const BacktraceFn id)
{
    if(id < BacktraceFn::first_valid_backtrace_fn_id ||
       id > BacktraceFn::last_valid_backtrace_fn_id)
    {
        os << "INVALID";
        return os;
    }

    switch(id)
    {
      case BacktraceFn::log:
        os << "log";
        break;

      case BacktraceFn::dump:
        os << "dump";
        break;
    }

    os << "()";

    return os;
}

class MockBacktrace::Expectation
{
  public:
    struct Data
    {
        const BacktraceFn function_id_;
        size_t arg_requested_depth_;
        std::string arg_message_;

      private:
        explicit Data(BacktraceFn fn, size_t arg_requested_depth,
                      std::string &&arg_message):
            function_id_(fn),
            arg_requested_depth_(arg_requested_depth),
            arg_message_(arg_message)
        {}

      public:
        explicit Data(BacktraceFn fn, size_t arg_requested_depth,
                      const char *arg_message):
            Data(fn, arg_requested_depth, std::string(arg_message))
        {}
    };

    const Data d;

  private:
    /* writable reference for simple ctor code */
    Data &data_ = *const_cast<Data *>(&d);

  public:
    Expectation(const Expectation &) = delete;
    Expectation(Expectation &&) = default;
    Expectation &operator=(const Expectation &) = delete;

    explicit Expectation(BacktraceFn fn, size_t arg_requested_depth,
                         const char *arg_message):
        d(fn, arg_requested_depth, arg_message)
    {}
};

MockBacktrace::MockBacktrace():
    expectations_(new MockExpectations())
{}

MockBacktrace::~MockBacktrace()
{
    delete expectations_;
}

void MockBacktrace::init()
{
    cppcut_assert_not_null(expectations_);
    expectations_->init();
}

void MockBacktrace::check() const
{
    cppcut_assert_not_null(expectations_);
    expectations_->check();
}

void MockBacktrace::expect_backtrace_log(size_t depth, const char *message)
{
    expectations_->add(Expectation(BacktraceFn::log, depth, message));
}

void MockBacktrace::expect_backtrace_dump(size_t depth, const char *message)
{
    expectations_->add(Expectation(BacktraceFn::dump, depth, message));
}

MockBacktrace *mock_backtrace_singleton = nullptr;

void backtrace_log(size_t depth, const char *message)
{
    const auto &expect(mock_backtrace_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, BacktraceFn::log);
    cppcut_assert_equal(expect.d.arg_requested_depth_, depth);
    cppcut_assert_equal(expect.d.arg_message_.c_str(), message);
}

void backtrace_dump(size_t depth, const char *message)
{
    const auto &expect(mock_backtrace_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, BacktraceFn::dump);
    cppcut_assert_equal(expect.d.arg_requested_depth_, depth);
    cppcut_assert_equal(expect.d.arg_message_.c_str(), message);
}
