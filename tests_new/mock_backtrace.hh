/*
 * Copyright (C) 2020, 2022, 2023  T+A elektroakustik GmbH & Co. KG
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
class Expectation: public MockExpectationBase
{
  public:
    Expectation(std::string &&name): MockExpectationBase(std::move(name)) {}
    virtual ~Expectation() {}
};

class Mock: public MockBase
{
  public:
    Mock(const Mock &) = delete;
    Mock &operator=(const Mock &) = delete;

    explicit Mock(std::shared_ptr<MockExpectationSequence> eseq = nullptr):
        MockBase("MockBacktrace", eseq)
    {}

    ~Mock() {}

    void expect(std::unique_ptr<Expectation> expectation)
    {
        add(std::move(expectation));
    }

    void expect(Expectation *expectation)
    {
        add(std::unique_ptr<Expectation>(expectation));
    }

    template <typename T, typename ... Args>
    auto &expect(Args ... args)
    {
        static_assert(std::is_base_of_v<Expectation, T> == true);
        return *static_cast<T *>(add(std::make_unique<T>(std::forward<Args>(args)...)));
    }

    template <typename T>
    void ignore(std::unique_ptr<Expectation> default_result)
    {
        ignore<T>(std::move(default_result));
    }

    template <typename T>
    void ignore(Expectation *default_result)
    {
        ignore<T>(std::unique_ptr<Expectation>(default_result));
    }
};


class LogOrDump: public Expectation
{
  private:
    const size_t requested_depth_;
    const std::string message_;

  protected:
    explicit LogOrDump(std::string &&name, size_t depth, const char *message):
        Expectation(std::move(name)),
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
        LogOrDump("Log", depth, message)
    {}

    static auto make_from_check_parameters(size_t depth, const char *message)
    {
        return std::make_unique<Log>(depth, message);
    }
};

class Dump: public LogOrDump
{
  public:
    explicit Dump(size_t depth = 0, const char *message = "bug context"):
        LogOrDump("Dump", depth, message)
    {}

    static auto make_from_check_parameters(size_t depth, const char *message)
    {
        return std::make_unique<Dump>(depth, message);
    }
};

extern Mock *singleton;

}

#endif /* MOCK_BACKTRACE_HH */
