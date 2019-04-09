/*
 * Copyright (C) 2017, 2018, 2019  T+A elektroakustik GmbH & Co. KG
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

#ifndef MAYBE_HH
#define MAYBE_HH

#include <utility>

template <typename T>
class Maybe
{
  private:
    bool is_value_known_;
    T value_;

  public:
    Maybe(const Maybe &) = default;
    Maybe &operator=(const Maybe &) = default;

    explicit Maybe():
        is_value_known_(false),
        value_()
    {}

    explicit Maybe(const T &value):
        is_value_known_(true),
        value_(value)
    {}

    explicit Maybe(T &&value):
        is_value_known_(true),
        value_(std::move(value))
    {}

    Maybe(Maybe &&src):
        is_value_known_(src.is_value_known_),
        value_(std::move(src.value_))
    {
        src.is_value_known_ = false;
    }

    Maybe &operator=(Maybe &&src)
    {
        is_value_known_ = src.is_value_known_;

        if(src.is_value_known_)
        {
            value_ = std::move(src.value_);
            src.is_value_known_ = false;
        }

        return *this;
    }

    void set_unknown()
    {
        is_value_known_ = false;
        value_ = T();
    }

    T &operator=(T &&value)
    {
        value_ = std::move(value);
        is_value_known_ = true;
        return value_;
    }

    T &operator=(const T &value)
    {
        value_ = value;
        is_value_known_ = true;
        return value_;
    }

    void set_known() { is_value_known_ = true; }
    bool is_known() const { return is_value_known_; }

    const T *operator->() const { return is_value_known_ ? &value_ : nullptr; }
    T *operator->() { return is_value_known_ ? &value_ : nullptr; }

    const T &get() const { return value_; }
    T &get_rw() { return value_; }

    const T &get(const T &if_unknown) const
    {
        return is_value_known_ ? value_ : if_unknown;
    }

    bool operator==(const T &other) const { return is_value_known_ ? value_ == other : false; }
    bool operator!=(const T &other) const { return is_value_known_ ? value_ != other : false; }

    bool operator==(const Maybe &other) const
    {
        if(other.is_value_known_)
            return *this == other.value_;
        else
            return !is_value_known_;
    }

    bool operator!=(const Maybe &other) const
    {
        if(other.is_value_known_)
            return *this != other.value_;
        else
            return is_value_known_;
    }

    template <typename TOut>
    const TOut &pick(const TOut &if_yes, const TOut &if_no, const TOut &if_unknown) const
    {
        return is_value_known_
            ? (value_ ? if_yes : if_no)
            : if_unknown;
    }
};

#endif /* !MAYBE_HH */
