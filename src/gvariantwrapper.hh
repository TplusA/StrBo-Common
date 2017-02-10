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

    static void set_ops(const Ops &ops);

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
