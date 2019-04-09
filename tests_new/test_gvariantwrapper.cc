/*
 * Copyright (C) 2018, 2019  T+A elektroakustik GmbH & Co. KG
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

#include <doctest.h>

#define GLIB_CHECK_VERSION

enum class RefMode
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
        is_floating(mode == RefMode::FLOATING)
    {}
};

typedef struct MockGVariant GVariant;

#include "gvariantwrapper.hh"

#include <iostream>

extern "C"
{
void g_variant_ref_sink(GVariant *) { FAIL("unexpected call"); }
void g_variant_ref(GVariant *) { FAIL("unexpected call"); }
void g_variant_unref(GVariant *) { FAIL("unexpected call"); }
int g_variant_is_floating(GVariant *) { FAIL("unexpected call"); return 0;}
void msg_error(int, int , const char *, ...) { FAIL("unexpected call"); }
void os_abort() { FAIL("unexpected call"); }
}

TEST_SUITE_BEGIN("GVariant wrapper tests");

class GVariantWrapperTestsFixture
{
  protected:
    const GVariantWrapper::Ops mock_ops = { sink, ref, unref, is_full_ref, };

  public:
    explicit GVariantWrapperTestsFixture()
    {
        GVariantWrapper::set_ops(mock_ops);
    }

  protected:
    static void sink(void *v)
    {
        auto *mv(static_cast<MockGVariant *>(v));

        REQUIRE(mv != nullptr);
        CHECK(mv->refs > 0);

        if(mv->is_floating)
            mv->is_floating = false;
        else
            ++mv->refs;
    }

    static void ref(void *v)
    {
        auto *mv(static_cast<MockGVariant *>(v));

        REQUIRE(mv != nullptr);
        CHECK(mv->refs > 0);

        ++mv->refs;
    }

    static void unref(void *&v)
    {
        auto *mv(static_cast<MockGVariant *>(v));

        REQUIRE(mv != nullptr);
        REQUIRE(mv->refs > 0);

        --mv->refs;

        if(mv->refs == 0)
            delete mv;

        v = nullptr;
    }

    static bool is_full_ref(void *v)
    {
        auto *mv(static_cast<MockGVariant *>(v));

        REQUIRE(mv != nullptr);
        CHECK(mv->refs > 0);

        return !mv->is_floating;
    }
};

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Default ctor manages nothing")
{
    GVariantWrapper w;

    CHECK(GVariantWrapper::get(w) == nullptr);
    CHECK(w == nullptr);
    CHECK_FALSE(w != nullptr);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Managed variant can be obtained")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);
    GVariantWrapper w(gv);

    CHECK(GVariantWrapper::get(w) == gv);
    CHECK_FALSE(w == nullptr);
    CHECK(w != nullptr);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Wrapper takes ownership of floating variant")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);

    CHECK(gv->refs == 1);
    CHECK(gv->is_floating);

    GVariantWrapper w(gv);

    CHECK(gv->refs == 1);
    CHECK_FALSE(gv->is_floating);
}

template <typename T>
static void multiple_wrappers_around_ref(GVariantWrapper &w,
                                         const MockGVariant *gv, const T &base,
                                         unsigned int expected_initial_refcount)
{
    CHECK(gv->refs == expected_initial_refcount);

    {
        GVariantWrapper b(base);
        CHECK(gv->refs == expected_initial_refcount + 1);

        GVariantWrapper c(base);
        CHECK(gv->refs == expected_initial_refcount + 2);

        GVariantWrapper d(base);
        CHECK(gv->refs == expected_initial_refcount + 3);
    }

    CHECK(gv->refs == expected_initial_refcount);
    CHECK_FALSE(gv->is_floating);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Multiple wrappers around single raw floating ref")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);
    CHECK(gv->refs == 1);
    CHECK(gv->is_floating);

    GVariantWrapper w(gv);

    multiple_wrappers_around_ref(w, gv, gv, 1);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Multiple wrappers around single raw full ref")
{
    auto *gv = new MockGVariant(RefMode::FULL);
    CHECK(gv->refs == 1);
    CHECK_FALSE(gv->is_floating);

    GVariantWrapper w(gv);

    multiple_wrappers_around_ref(w, gv, gv, 2);

    void *temp = static_cast<void *>(gv);
    unref(temp);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Multiple wrappers around full raw ref with move semantics")
{
    auto *gv = new MockGVariant(RefMode::FULL);
    CHECK(gv->refs == 1);
    CHECK_FALSE(gv->is_floating);

    GVariantWrapper w(gv, GVariantWrapper::Transfer::JUST_MOVE);

    multiple_wrappers_around_ref(w, gv, gv, 1);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Multiple wrappers around floating ref via copy ctor")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);
    CHECK(gv->refs == 1);
    CHECK(gv->is_floating);

    GVariantWrapper w(gv);

    multiple_wrappers_around_ref(w, gv, w, 1);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Multiple wrappers around full ref via copy ctor")
{
    auto *gv = new MockGVariant(RefMode::FULL);
    CHECK(gv->refs == 1);
    CHECK_FALSE(gv->is_floating);

    GVariantWrapper w(gv);

    multiple_wrappers_around_ref(w, gv, w, 2);

    void *temp = static_cast<void *>(gv);
    unref(temp);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Multiple wrappers around full copied ref with move semantics")
{
    auto *gv = new MockGVariant(RefMode::FULL);
    CHECK(gv->refs == 1);
    CHECK_FALSE(gv->is_floating);

    GVariantWrapper w(gv, GVariantWrapper::Transfer::JUST_MOVE);

    multiple_wrappers_around_ref(w, gv, w, 1);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Explicit removal of managed variant")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);

    GVariantWrapper w(gv);

    CHECK(gv->refs == 1);
    CHECK(GVariantWrapper::get(w) == gv);

    w.release();

    CHECK(GVariantWrapper::get(w) == nullptr);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Explicit removal of multiply managed variant")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);

    GVariantWrapper w(gv);

    CHECK(gv->refs == 1);
    CHECK(GVariantWrapper::get(w) == gv);

    {
        GVariantWrapper v(w);

        CHECK(gv->refs == 2);
        CHECK(GVariantWrapper::get(v) == gv);
        CHECK(GVariantWrapper::get(w) == gv);

        v.release();

        CHECK(gv->refs == 1);
        CHECK(GVariantWrapper::get(v) == nullptr);
        CHECK(GVariantWrapper::get(w) == gv);
    }

    w.release();

    CHECK(GVariantWrapper::get(w) == nullptr);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Copy assign wrappers")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);

    GVariantWrapper a(gv);

    CHECK(GVariantWrapper::get(a) == gv);
    CHECK(gv->refs == 1);

    GVariantWrapper b;
    CHECK(GVariantWrapper::get(b) == nullptr);

    b = a;

    CHECK(gv->refs == 2);
    CHECK(GVariantWrapper::get(a) == gv);
    CHECK(GVariantWrapper::get(b) == gv);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Copy assign to self")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);

    GVariantWrapper a(gv);

    CHECK(GVariantWrapper::get(a) == gv);
    CHECK(gv->refs == 1);

    a = a;

    CHECK(GVariantWrapper::get(a) == gv);
    CHECK(gv->refs == 1);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Move assign wrappers")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);

    GVariantWrapper a(gv);

    CHECK(GVariantWrapper::get(a) == gv);
    CHECK(gv->refs == 1);

    GVariantWrapper b;
    CHECK(GVariantWrapper::get(b) == nullptr);

    b = std::move(a);

    CHECK(gv->refs == 1);
    CHECK(GVariantWrapper::get(a) == nullptr);
    CHECK(GVariantWrapper::get(b) == gv);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Move assign to self")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);

    GVariantWrapper a(gv);

    CHECK(GVariantWrapper::get(a) == gv);
    CHECK(gv->refs == 1);

    a = std::move(a);

    CHECK(GVariantWrapper::get(a) == gv);
    CHECK(gv->refs == 1);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Move construct wrappers")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);

    GVariantWrapper a(gv);

    CHECK(GVariantWrapper::get(a) == gv);
    CHECK(gv->refs == 1);

    GVariantWrapper b(std::move(a));

    CHECK(gv->refs == 1);
    CHECK(GVariantWrapper::get(a) == nullptr);
    CHECK(GVariantWrapper::get(b) == gv);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Take wrapped variant from wrapper")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);
    REQUIRE(gv != nullptr);

    GVariantWrapper a(gv);

    auto *taken = GVariantWrapper::move(a);

    CHECK(GVariantWrapper::get(a) == nullptr);
    CHECK(taken == gv);
    CHECK(gv->refs == 1);

    void *temp = static_cast<void *>(gv);
    unref(temp);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Take wrapped variant from multiple wrappers")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);
    REQUIRE(gv != nullptr);

    GVariantWrapper a(gv);
    GVariantWrapper b(gv);
    GVariantWrapper c(b);

    auto *taken = GVariantWrapper::move(a);

    CHECK(GVariantWrapper::get(a) == nullptr);
    CHECK(GVariantWrapper::get(b) == gv);
    CHECK(GVariantWrapper::get(c) == gv);
    CHECK(taken == gv);
    CHECK(gv->refs == 3);

    void *temp = static_cast<void *>(gv);
    unref(temp);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Equality of empty default constructed wrappers")
{
    GVariantWrapper a;
    GVariantWrapper b;

    CHECK(a == b);
    CHECK(b == a);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Equality of wrappers around same variant")
{
    auto *gv = new MockGVariant(RefMode::FLOATING);
    REQUIRE(gv != nullptr);

    GVariantWrapper a(gv);
    GVariantWrapper b(gv);

    CHECK(a == b);
    CHECK(b == a);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Unequality of empty and loaded wrappers")
{
    GVariantWrapper a;
    GVariantWrapper b(new MockGVariant(RefMode::FLOATING));

    CHECK_FALSE(a == b);
    CHECK_FALSE(b == a);
}

TEST_CASE_FIXTURE(GVariantWrapperTestsFixture, "Unequality of wrappers around different variants")
{
    auto *gva = new MockGVariant(RefMode::FLOATING);
    auto *gvb = new MockGVariant(RefMode::FLOATING);

    CHECK(gva != gvb);
    REQUIRE(gva != nullptr);

    GVariantWrapper a(gva);
    GVariantWrapper b(gvb);

    CHECK_FALSE(a == b);
    CHECK_FALSE(b == a);
}

TEST_SUITE_END();
