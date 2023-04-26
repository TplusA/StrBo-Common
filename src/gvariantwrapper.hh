/*
 * Copyright (C) 2017, 2018, 2019, 2023  T+A elektroakustik GmbH & Co. KG
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

#ifndef GVARIANTWRAPPER_HH
#define GVARIANTWRAPPER_HH

#include <cstddef>

class GVariantWrapper
{
  public:
    struct Ops
    {
        void (*const sink)(void *v);
        void (*const ref)(void *v);
        void (*const unref)(void *&v);
        bool (*const is_full_reference)(void *v);
    };

    enum class Transfer
    {
        TAKE_OWNERSHIP,
        JUST_MOVE,
    };

  private:
    void *variant_;

  public:
    GVariantWrapper(const GVariantWrapper &);
    GVariantWrapper(GVariantWrapper &&);
    GVariantWrapper &operator=(const GVariantWrapper &);
    GVariantWrapper &operator=(GVariantWrapper &&);

  private:
    explicit GVariantWrapper(void *variant, Transfer transfer);

  public:
    explicit GVariantWrapper():
        variant_(nullptr)
    {}

    ~GVariantWrapper()
    {
        release();
    }

  public:
    void release();

    bool operator==(std::nullptr_t n) const { return variant_ == nullptr; }
    bool operator!=(std::nullptr_t n) const { return variant_ != nullptr; }

    bool operator==(const GVariantWrapper &other) const
    {
        return variant_ == other.variant_;
    }

    static void set_ops(const Ops &o);

    /*!
     * Use this for unit tests ONLY! NEVER call this from production code!
     */
    size_t get_ref_count() const;

#ifdef GLIB_CHECK_VERSION
  public:
    explicit GVariantWrapper(GVariant *variant,
                             Transfer transfer = Transfer::TAKE_OWNERSHIP):
        GVariantWrapper(static_cast<void *>(variant), transfer)
    {}

    static GVariant *get(const GVariantWrapper &w)
    {
        return static_cast<GVariant *>(w.variant_);
    }

    static GVariant *move(GVariantWrapper &w)
    {
        auto *temp(get(w));
        w.variant_ = nullptr;
        return temp;
    }
#endif /* GLIB_CHECK_VERSION */
};

#endif /* !GVARIANTWRAPPER_HH */
