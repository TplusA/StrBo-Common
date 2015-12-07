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

#ifndef ERROR_THROWER_HH
#define ERROR_THROWER_HH

#include <stdexcept>
#include <sstream>

/*!
 * Throw an exception, passing a constructed string to its constructor.
 *
 * This class template is a RAII-style helper for throwing exceptions whose
 * constructors expect a string parameter. The class template implements the
 * formatted input operator which allows client code to throw an exception with
 * a formatted string parameter using a single line of code, without having to
 * create temporaries or having to deal with \c std::ostringstream.
 *
 * The exception is thrown from the destructor. This is something which should
 * usually be avoided in C++, but in this very narrow scope it is a safe thing
 * to do. At last, throwing an exception is the whole purpose of this class
 * template.
 *
 * By default, this class template will throw a \c std::runtime_error
 * exception. Any other exception type may be passed as template parameter.
 */
template <typename EType = std::runtime_error>
class ErrorThrower
{
  private:
    std::ostringstream os_;

  public:
    ErrorThrower(const ErrorThrower &) = delete;
    ErrorThrower(ErrorThrower &&) = default;
    ErrorThrower &operator=(const ErrorThrower &) = delete;
    ErrorThrower &operator=(ErrorThrower &&) = default;

    explicit ErrorThrower() = default;

    // cppcheck-suppress exceptThrowInDestructor
    [[ noreturn ]] ~ErrorThrower() noexcept(false) { throw EType(os_.str()); }

    template <typename T>
    ErrorThrower &operator<<(const T &d)
    {
        os_ << d;
        return *this;
    }
};

#endif /* !ERROR_THROWER_HH */
