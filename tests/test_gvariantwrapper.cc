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

#include <cppcutter.h>

#define GLIB_CHECK_VERSION

enum RefMode
{
    FLOATING,
    FULL,
};

struct MockGVariant
{
    unsigned int refs;
    bool is_floating;

    MockGVariant(RefMode mode):
        refs(1),
        is_floating(mode == FLOATING)
    {}
};

typedef struct MockGVariant GVariant;

#include "gvariantwrapper.hh"

namespace gvariantwrapper_tests
{

static void sink(void *v)
{
    auto *mv(static_cast<MockGVariant *>(v));

    cppcut_assert_not_null(mv);
    cppcut_assert_not_equal(0U, mv->refs);

    if(mv->is_floating)
        mv->is_floating = false;
    else
        ++mv->refs;
}

static void ref(void *v)
{
    auto *mv(static_cast<MockGVariant *>(v));

    cppcut_assert_not_null(mv);
    cppcut_assert_not_equal(0U, mv->refs);

    ++mv->refs;
}

static void unref(void *&v)
{
    auto *mv(static_cast<MockGVariant *>(v));

    cppcut_assert_not_null(mv);
    cppcut_assert_not_equal(0U, mv->refs);

    --mv->refs;

    if(mv->refs == 0)
        delete mv;

    v = nullptr;
}

static bool is_full_ref(void *v)
{
    auto *mv(static_cast<MockGVariant *>(v));

    cppcut_assert_not_null(mv);
    cppcut_assert_not_equal(0U, mv->refs);

    return !mv->is_floating;
}

static const GVariantWrapper::Ops mock_ops = { sink, ref, unref, is_full_ref, };

void cut_setup()
{
    GVariantWrapper::set_ops(mock_ops);
}

void test_default_ctor_manages_nothing()
{
    GVariantWrapper w;

    cppcut_assert_null(GVariantWrapper::get(w));
    cut_assert_true(w == nullptr);
    cut_assert_false(w != nullptr);
}

void test_managed_variant_can_be_obtained()
{
    auto *gv = new MockGVariant(FLOATING);
    GVariantWrapper w(gv);

    cppcut_assert_equal(gv, GVariantWrapper::get(w));
    cut_assert_false(w == nullptr);
    cut_assert_true(w != nullptr);
}

void test_wrapper_takes_ownership_of_floating_variant()
{
    auto *gv = new MockGVariant(FLOATING);

    cppcut_assert_equal(1U, gv->refs);
    cut_assert_true(gv->is_floating);

    GVariantWrapper w(gv);

    cppcut_assert_equal(1U, gv->refs);
    cut_assert_false(gv->is_floating);
}

template <typename T>
static void multiple_wrappers_around_ref(GVariantWrapper &w,
                                         const MockGVariant *gv, const T &base,
                                         unsigned int expected_initial_refcount)
{
    cppcut_assert_equal(expected_initial_refcount, gv->refs);

    {
        GVariantWrapper b(base);
        cppcut_assert_equal(expected_initial_refcount + 1, gv->refs);

        GVariantWrapper c(base);
        cppcut_assert_equal(expected_initial_refcount + 2, gv->refs);

        GVariantWrapper d(base);
        cppcut_assert_equal(expected_initial_refcount + 3, gv->refs);
    }

    cppcut_assert_equal(expected_initial_refcount, gv->refs);
    cut_assert_false(gv->is_floating);
}

void test_multiple_wrappers_around_single_raw_floating_ref()
{
    auto *gv = new MockGVariant(FLOATING);
    cppcut_assert_equal(1U, gv->refs);
    cut_assert_true(gv->is_floating);

    GVariantWrapper w(gv);

    multiple_wrappers_around_ref(w, gv, gv, 1);
}

void test_multiple_wrappers_around_single_raw_full_ref()
{
    auto *gv = new MockGVariant(FULL);
    cppcut_assert_equal(1U, gv->refs);
    cut_assert_false(gv->is_floating);

    GVariantWrapper w(gv);

    multiple_wrappers_around_ref(w, gv, gv, 2);

    void *temp = static_cast<void *>(gv);
    unref(temp);
}

void test_multiple_wrappers_around_full_raw_ref_with_move_semantics()
{
    auto *gv = new MockGVariant(FULL);
    cppcut_assert_equal(1U, gv->refs);
    cut_assert_false(gv->is_floating);

    GVariantWrapper w(gv, GVariantWrapper::Transfer::JUST_MOVE);

    multiple_wrappers_around_ref(w, gv, gv, 1);
}

void test_multiple_wrappers_around_floating_ref_via_copy_ctor()
{
    auto *gv = new MockGVariant(FLOATING);
    cppcut_assert_equal(1U, gv->refs);
    cut_assert_true(gv->is_floating);

    GVariantWrapper w(gv);

    multiple_wrappers_around_ref(w, gv, w, 1);
}

void test_multiple_wrappers_around_full_ref_via_copy_ctor()
{
    auto *gv = new MockGVariant(FULL);
    cppcut_assert_equal(1U, gv->refs);
    cut_assert_false(gv->is_floating);

    GVariantWrapper w(gv);

    multiple_wrappers_around_ref(w, gv, w, 2);

    void *temp = static_cast<void *>(gv);
    unref(temp);
}

void test_multiple_wrappers_around_full_copied_ref_with_move_semantics()
{
    auto *gv = new MockGVariant(FULL);
    cppcut_assert_equal(1U, gv->refs);
    cut_assert_false(gv->is_floating);

    GVariantWrapper w(gv, GVariantWrapper::Transfer::JUST_MOVE);

    multiple_wrappers_around_ref(w, gv, w, 1);
}

void test_explicit_removal_of_managed_variant()
{
    auto *gv = new MockGVariant(FLOATING);

    GVariantWrapper w(gv);

    cppcut_assert_equal(1U, gv->refs);
    cppcut_assert_equal(gv, GVariantWrapper::get(w));

    w.release();

    cppcut_assert_null(GVariantWrapper::get(w));
}

void test_explicit_removal_of_multiply_managed_variant()
{
    auto *gv = new MockGVariant(FLOATING);

    GVariantWrapper w(gv);

    cppcut_assert_equal(1U, gv->refs);
    cppcut_assert_equal(gv, GVariantWrapper::get(w));

    {
        GVariantWrapper v(w);

        cppcut_assert_equal(2U, gv->refs);
        cppcut_assert_equal(gv, GVariantWrapper::get(v));
        cppcut_assert_equal(gv, GVariantWrapper::get(w));

        v.release();

        cppcut_assert_equal(1U, gv->refs);
        cppcut_assert_null(GVariantWrapper::get(v));
        cppcut_assert_equal(gv, GVariantWrapper::get(w));
    }

    w.release();

    cppcut_assert_null(GVariantWrapper::get(w));
}

void test_copy_assign_wrappers()
{
    auto *gv = new MockGVariant(FLOATING);

    GVariantWrapper a(gv);

    cppcut_assert_equal(gv, GVariantWrapper::get(a));
    cppcut_assert_equal(1U, gv->refs);

    GVariantWrapper b;
    cppcut_assert_null(GVariantWrapper::get(b));

    b = a;

    cppcut_assert_equal(2U, gv->refs);
    cppcut_assert_equal(gv, GVariantWrapper::get(a));
    cppcut_assert_equal(gv, GVariantWrapper::get(b));
}

void test_copy_assign_to_self()
{
    auto *gv = new MockGVariant(FLOATING);

    GVariantWrapper a(gv);

    cppcut_assert_equal(gv, GVariantWrapper::get(a));
    cppcut_assert_equal(1U, gv->refs);

    a = a;

    cppcut_assert_equal(gv, GVariantWrapper::get(a));
    cppcut_assert_equal(1U, gv->refs);
}

void test_move_assign_wrappers()
{
    auto *gv = new MockGVariant(FLOATING);

    GVariantWrapper a(gv);

    cppcut_assert_equal(gv, GVariantWrapper::get(a));
    cppcut_assert_equal(1U, gv->refs);

    GVariantWrapper b;
    cppcut_assert_null(GVariantWrapper::get(b));

    b = std::move(a);

    cppcut_assert_equal(1U, gv->refs);
    cppcut_assert_null(GVariantWrapper::get(a));
    cppcut_assert_equal(gv, GVariantWrapper::get(b));
}

void test_move_assign_to_self()
{
    auto *gv = new MockGVariant(FLOATING);

    GVariantWrapper a(gv);

    cppcut_assert_equal(gv, GVariantWrapper::get(a));
    cppcut_assert_equal(1U, gv->refs);

    a = std::move(a);

    cppcut_assert_equal(gv, GVariantWrapper::get(a));
    cppcut_assert_equal(1U, gv->refs);
}

void test_move_construct_wrappers()
{
    auto *gv = new MockGVariant(FLOATING);

    GVariantWrapper a(gv);

    cppcut_assert_equal(gv, GVariantWrapper::get(a));
    cppcut_assert_equal(1U, gv->refs);

    GVariantWrapper b(std::move(a));

    cppcut_assert_equal(1U, gv->refs);
    cppcut_assert_null(GVariantWrapper::get(a));
    cppcut_assert_equal(gv, GVariantWrapper::get(b));
}

void test_take_wrapped_variant_from_wrapper()
{
    auto *gv = new MockGVariant(FLOATING);
    cppcut_assert_not_null(gv);

    GVariantWrapper a(gv);

    auto *taken = GVariantWrapper::move(a);

    cppcut_assert_null(GVariantWrapper::get(a));
    cppcut_assert_equal(gv, taken);
    cppcut_assert_equal(1U, gv->refs);

    void *temp = static_cast<void *>(gv);
    unref(temp);
}

void test_take_wrapped_variant_from_multiple_wrappers()
{
    auto *gv = new MockGVariant(FLOATING);
    cppcut_assert_not_null(gv);

    GVariantWrapper a(gv);
    GVariantWrapper b(gv);
    GVariantWrapper c(b);

    auto *taken = GVariantWrapper::move(a);

    cppcut_assert_null(GVariantWrapper::get(a));
    cppcut_assert_equal(gv, GVariantWrapper::get(b));
    cppcut_assert_equal(gv, GVariantWrapper::get(c));
    cppcut_assert_equal(gv, taken);
    cppcut_assert_equal(3U, gv->refs);

    void *temp = static_cast<void *>(gv);
    unref(temp);
}

void test_equality_of_empty_default_constructed_wrappers()
{
    GVariantWrapper a;
    GVariantWrapper b;

    cut_assert_true(a == b);
    cut_assert_true(b == a);
}

void test_equality_of_wrappers_around_same_variant()
{
    auto *gv = new MockGVariant(FLOATING);
    cppcut_assert_not_null(gv);

    GVariantWrapper a(gv);
    GVariantWrapper b(gv);

    cut_assert_true(a == b);
    cut_assert_true(b == a);
}

void test_unequality_of_empty_and_loaded_wrappers()
{
    GVariantWrapper a;
    GVariantWrapper b(new MockGVariant(FLOATING));

    cut_assert_false(a == b);
    cut_assert_false(b == a);
}

void test_unequality_of_wrappers_around_different_variants()
{
    auto *gva = new MockGVariant(FLOATING);
    auto *gvb = new MockGVariant(FLOATING);

    cppcut_assert_not_equal(gva, gvb);
    cppcut_assert_not_null(gva);

    GVariantWrapper a(gva);
    GVariantWrapper b(gvb);

    cut_assert_false(a == b);
    cut_assert_false(b == a);
}

}
