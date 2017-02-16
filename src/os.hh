/*
 * Copyright (C) 2017  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of the T+A Streaming Board software stack ("StrBoWare").
 *
 * StrBoWare is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3 as
 * published by the Free Software Foundation.
 *
 * StrBoWare is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StrBoWare.  If not, see <http://www.gnu.org/licenses/>.
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

};

#endif /* !OS_HH */
