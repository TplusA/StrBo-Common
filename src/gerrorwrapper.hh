/*
 * Copyright (C) 2019  T+A elektroakustik GmbH & Co. KG
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

#ifndef GERRORWRAPPER_HH
#define GERRORWRAPPER_HH

#include "messages.h"

#include <glib.h>

class GErrorWrapper
{
  private:
    GError *gerror_;
    bool is_noticed_;

  public:
    GErrorWrapper(const GErrorWrapper &) = delete;
    GErrorWrapper(GErrorWrapper &&) = delete;
    GErrorWrapper &operator=(const GErrorWrapper &) = delete;
    GErrorWrapper &operator=(GErrorWrapper &&) = delete;

    explicit GErrorWrapper():
        gerror_(nullptr),
        is_noticed_(false)
    {}

    ~GErrorWrapper() { free_error(true); }

    GError **await()
    {
        free_error(false);

        gerror_ = nullptr;
        is_noticed_ = false;

        return &gerror_;
    }

    void noticed() { is_noticed_ = true; }

    bool failed() const { return gerror_ != nullptr; }

    bool log_failure(const char *what)
    {
        if(gerror_ == nullptr)
            return false;

        if(what == nullptr)
            what = "<UNKNOWN>";

        if(gerror_->message != nullptr)
            msg_error(0, LOG_EMERG, "%s: Got %s error: %s",
                      what, g_quark_to_string(gerror_->domain),
                      gerror_->message);
        else
            msg_error(0, LOG_EMERG, "%s: Got %s error without any message",
                      what, g_quark_to_string(gerror_->domain));

        noticed();

        return true;
    }

    const GError *operator->() const { return gerror_; }

  private:
    void free_error(bool from_dtor)
    {
        if(gerror_ == nullptr)
            return;

        if(!is_noticed_)
            BUG("Unhandled error %s ('%s', %d): %s",
                from_dtor ? "went out of scope" : "overwritten",
                g_quark_to_string(gerror_->domain), gerror_->code,
                gerror_->message);

        g_error_free(gerror_);
    }
};

#endif /* !GERRORWRAPPER_HH */
