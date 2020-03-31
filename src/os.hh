/*
 * Copyright (C) 2017, 2019, 2020  T+A elektroakustik GmbH & Co. KG
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

#ifndef OS_HH
#define OS_HH

#include "os.h"

namespace OS
{

class SuppressErrorsGuard
{
  private:
    const bool previous_state_;
    bool state_;

  public:
    SuppressErrorsGuard(const SuppressErrorsGuard &) = delete;
    SuppressErrorsGuard &operator=(const SuppressErrorsGuard &) = delete;

    explicit SuppressErrorsGuard(bool do_suppress = true):
        previous_state_(os_suppress_error_messages(do_suppress)),
        state_(do_suppress)
    {}

    ~SuppressErrorsGuard()
    {
        os_suppress_error_messages(previous_state_);
    }

    bool toggle()
    {
        state_ = os_suppress_error_messages(!state_);
        return state_;
    }
};

}

#endif /* !OS_HH */
