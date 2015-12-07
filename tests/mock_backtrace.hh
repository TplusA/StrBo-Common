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

class MockBacktrace
{
  public:
    MockBacktrace(const MockBacktrace &) = delete;
    MockBacktrace &operator=(const MockBacktrace &) = delete;

    class Expectation;
    typedef MockExpectationsTemplate<Expectation> MockExpectations;
    MockExpectations *expectations_;

    explicit MockBacktrace();
    ~MockBacktrace();

    void init();
    void check() const;

    void expect_backtrace_log(size_t depth = 0, const char *message = "bug context");
    void expect_backtrace_dump(size_t depth = 0, const char *message = "bug context");
};

extern MockBacktrace *mock_backtrace_singleton;

#endif /* !MOCK_BACKTRACE_HH */
