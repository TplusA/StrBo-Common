/*
 * Copyright (C) 2017, 2019, 2022, 2023  T+A elektroakustik GmbH & Co. KG
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

/*!
 * HACK! Copied from glib's gvariant-core.c and modified a bit.
 */
struct _GVariantInternal
{
    void *type_info;
    gsize size;

    union
    {
        struct
        {
            GBytes *bytes;
            gconstpointer data;

#if GLIB_CHECK_VERSION(2, 75, 1)
            gsize ordered_offsets_up_to;
            gsize checked_offsets_up_to;
#endif /* 2.75.1 or later */
        }
        serialised;

        struct
        {
            GVariant **children;
            gsize n_children;
        }
        tree;
    }
    contents;

    gint state;
    gatomicrefcount ref_count;
    gsize depth;
};

size_t GVariantWrapper::get_ref_count() const
{
    if(variant_ == nullptr)
        return 0;

    const auto *gv = reinterpret_cast<const _GVariantInternal *>(variant_);
    return gv->ref_count;
}

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
        msg_log_assert(is_full_reference(variant));
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
