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

#ifndef MOCK_BACKTRACE_HH
#define MOCK_BACKTRACE_HH

#include "backtrace.h"
#include "mock_expectation.hh"

namespace MockBacktrace
{

/*! Base class for expectations. */
class Expectation
{
  public:
    Expectation(const Expectation &) = delete;
    Expectation(Expectation &&) = default;
    Expectation &operator=(const Expectation &) = delete;
    Expectation &operator=(Expectation &&) = default;
    Expectation() {}
    virtual ~Expectation() {}
};

class Mock
{
  private:
    MockExpectationsTemplate<Expectation> expectations_;

  public:
    Mock(const Mock &) = delete;
    Mock &operator=(const Mock &) = delete;

    explicit Mock():
        expectations_("MockBacktrace")
    {}

    ~Mock() {}

    void expect(std::unique_ptr<Expectation> expectation)
    {
        expectations_.add(std::move(expectation));
    }

    void expect(Expectation *expectation)
    {
        expectations_.add(std::unique_ptr<Expectation>(expectation));
    }

    template <typename T>
    void ignore(std::unique_ptr<T> default_result)
    {
        expectations_.ignore<T>(std::move(default_result));
    }

    template <typename T>
    void ignore(T *default_result)
    {
        expectations_.ignore<T>(std::unique_ptr<Expectation>(default_result));
    }

    template <typename T>
    void allow() { expectations_.allow<T>(); }

    void done() const { expectations_.done(); }

    template <typename T, typename ... Args>
    auto check_next(Args ... args) -> decltype(std::declval<T>().check(args...))
    {
        return expectations_.check_and_advance<T, decltype(std::declval<T>().check(args...))>(args...);
    }

    template <typename T>
    const T &next(const char *caller)
    {
        return expectations_.next<T>(caller);
    }
};


class LogOrDump: public Expectation
{
  private:
    const size_t requested_depth_;
    const std::string message_;

  protected:
    explicit LogOrDump(size_t depth, const char *message):
        requested_depth_(depth),
        message_(message != nullptr ? message : "")
    {}

  public:
    void check(size_t depth, const char *message) const
    {
        CHECK(depth == requested_depth_);

        if(message == nullptr)
            CHECK(message_.empty());
        else
            CHECK(std::string(message) == message_);
    }
};

class Log: public LogOrDump
{
  public:
    explicit Log(size_t depth = 0, const char *message = "bug context"):
        LogOrDump(depth, message)
    {}
};

class Dump: public LogOrDump
{
  public:
    explicit Dump(size_t depth = 0, const char *message = "bug context"):
        LogOrDump(depth, message)
    {}
};

extern Mock *singleton;

}

#endif /* MOCK_BACKTRACE_HH */
