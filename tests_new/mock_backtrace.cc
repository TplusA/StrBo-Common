/*
 * Copyright (C) 2020, 2022  T+A elektroakustik GmbH & Co. KG
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

#include "mock_backtrace.hh"

MockBacktrace::Mock *MockBacktrace::singleton = nullptr;


void backtrace_log(size_t depth, const char *message)
{
    REQUIRE(MockBacktrace::singleton != nullptr);
    MockBacktrace::singleton->check_next<MockBacktrace::Log>(depth, message);
}

void backtrace_dump(size_t depth, const char *message)
{
    REQUIRE(MockBacktrace::singleton != nullptr);
    MockBacktrace::singleton->check_next<MockBacktrace::Dump>(depth, message);
}
