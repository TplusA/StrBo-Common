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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <glib.h>

#include "gvariantwrapper.hh"
#include "messages.h"

static void sink_op(void *v)
{
    g_variant_ref_sink(static_cast<GVariant *>(v));
}

static void ref_op(void *v)
{
    g_variant_ref(static_cast<GVariant *>(v));
}

static void unref_op(void *&v)
{
    g_variant_unref(static_cast<GVariant *>(v));
    v = nullptr;
}

static bool is_full_reference_op(void *v)
{
    return !g_variant_is_floating(static_cast<GVariant *>(v));
}

static const GVariantWrapper::Ops default_ops =
{
    sink_op, ref_op, unref_op, is_full_reference_op,
};

static const GVariantWrapper::Ops *ops = &default_ops;

void GVariantWrapper::set_ops(const GVariantWrapper::Ops &o) { ops = &o; }

static inline void sink(void *variant)
{
    if(variant != nullptr)
        ops->sink(variant);
}

static inline void ref(void *variant)
{
    if(variant != nullptr)
        ops->ref(variant);
}

static inline void unref(void *&variant)
{
    if(variant != nullptr)
        ops->unref(variant);
}

static inline bool is_full_reference(void *variant)
{
    return (variant != nullptr)
        ? ops->is_full_reference(variant)
        : true;
}


GVariantWrapper::GVariantWrapper(void *variant, GVariantWrapper::Transfer transfer):
    variant_(variant)
{
    switch(transfer)
    {
      case Transfer::TAKE_OWNERSHIP:
        sink(variant_);
        break;

      case Transfer::JUST_MOVE:
        log_assert(is_full_reference(variant));
        break;
    }
}

GVariantWrapper::GVariantWrapper(const GVariantWrapper &src):
    variant_(src.variant_)
{
    ref(variant_);
}

GVariantWrapper::GVariantWrapper(GVariantWrapper &&src):
    variant_(src.variant_)
{
    src.variant_ = nullptr;
}

GVariantWrapper &GVariantWrapper::operator=(const GVariantWrapper &src)
{
    if(variant_ != src.variant_)
    {
        unref(variant_);
        variant_ = src.variant_;
        ref(variant_);
    }

    return *this;
}

GVariantWrapper &GVariantWrapper::operator=(GVariantWrapper &&src)
{
    if(this != &src)
    {
        unref(variant_);
        variant_ = src.variant_;
        src.variant_ = nullptr;
    }

    return *this;
}

void GVariantWrapper::release()
{
    unref(variant_);
}
