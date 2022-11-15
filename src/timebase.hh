/*
 * Copyright (C) 2015, 2019, 2022  T+A elektroakustik GmbH & Co. KG
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

#ifndef TIMEBASE_HH
#define TIMEBASE_HH

#include <chrono>

/*!
 * Interface with default implementation for obtaining time stamps.
 *
 * Functions may be overloaded by unit test code so that full, precise control
 * over time becomes possible during tests.
 */
class Timebase
{
  private:
    using clock = std::chrono::steady_clock;

  public:
    using time_point = clock::time_point;

    Timebase(const Timebase &) = delete;
    Timebase &operator=(const Timebase &) = delete;

    explicit Timebase() {}
    virtual ~Timebase() {}

    virtual time_point now() const { return clock::now(); }
};

#endif /* !TIMEBASE_HH */
