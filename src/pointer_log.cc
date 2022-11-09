/*
 * Copyright (C) 2022  T+A elektroakustik GmbH & Co. KG
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

#include "pointer_log.hh"
#include "messages.h"

#include <cstdio>

static void fill_extra_info(std::array<char, 1024> &buffer, const char *annotation,
                            const char *ctx, const char *file, const char *fn, int line)
{
    if(annotation == nullptr)
        annotation = "";

    if(ctx == nullptr && file == nullptr)
    {
        if(annotation[0] == '\0')
            buffer[0] = '\0';
        else
            std::snprintf(buffer.data(), buffer.size(), "%s",annotation + 1);
    }
    else if(ctx != nullptr && file == nullptr)
        std::snprintf(buffer.data(), buffer.size(),
                      "\"%s\"%s", ctx, annotation);
    else if(ctx == nullptr && file != nullptr)
        std::snprintf(buffer.data(), buffer.size(),
                      "%s @%s:%s(%d)", annotation, file, fn, line);
    else
        std::snprintf(buffer.data(), buffer.size(),
                      "\"%s\"%s @%s:%s(%d)", ctx, annotation, file, fn, line);
}

static void do_show(const void *vp, const PointerLog::MapType &pointers,
                    const PointerLog::MapType::const_iterator &it,
                    const char *extra_info)
{
    if(it != pointers.end())
        msg_info("PointerLog: [%s(%p) %s:%s(%d)] %s",
                 std::get<0>(it->second).c_str(), it->first,
                 std::get<1>(it->second).c_str(), std::get<2>(it->second).c_str(),
                 std::get<3>(it->second), extra_info);
    else
        msg_info("PointerLog: [%p ***UNKNOWN***] %s", vp, extra_info);
}

static const char *basename(const char *n)
{
    if(n == nullptr)
        return nullptr;

    const char *result = n;
    for(const char *it = n; *it != '\0'; ++it)
        if(*it == '/')
            result = it + 1;

    return result;
}

void PointerLog::add_untyped(const void *vp, const char *name, const char *ctx,
                             const char *file, const char *fn, int line)
{
    file = basename(file);

    if(vp == nullptr)
    {
        MSG_BUG("Tried to add nullptr to PointerLog at %s:%s(%d)", file, fn, line);
        return;
    }

    std::array<char, 1024> extra_info;
    auto it(pointers_.find(vp));

    if(it != pointers_.end())
    {
        fill_extra_info(extra_info, " ***REPLACED***", ctx, file, fn, line);
        do_show(vp, pointers_, it, extra_info.data());
        it->second = std::make_tuple(name, file, fn, line);
    }
    else
    {
        auto result(pointers_.emplace(vp, std::make_tuple(name, file, fn, line)));
        fill_extra_info(extra_info, " ***NEW***", ctx, file, fn, line);
        do_show(vp, pointers_, result.first, extra_info.data());
    }
}

void PointerLog::remove_untyped(const void *vp, const char *ctx,
                                const char *file, const char *fn, int line)
{
    file = basename(file);

    if(vp == nullptr)
    {
        MSG_BUG("Tried to remove nullptr from PointerLog at %s:%s(%d)", file, fn, line);
        return;
    }

    std::array<char, 1024> extra_info;
    fill_extra_info(extra_info, " ***REMOVED***", ctx, file, fn, line);

    auto it(pointers_.find(vp));
    do_show(vp, pointers_, it, extra_info.data());

    if(it != pointers_.end())
        pointers_.erase(it);
}

void PointerLog::show_untyped(const void *vp, const char *ctx,
                              const char *file, const char *fn, int line)
{
    if(vp == nullptr)
        return;

    std::array<char, 1024> extra_info;
    fill_extra_info(extra_info, nullptr, ctx, basename(file), fn, line);
    do_show(vp, pointers_, pointers_.find(vp), extra_info.data());
}

void PointerLog::show_all(const char *ctx, const char *file, const char *fn, int line)
{
    std::array<char, 1024> extra_info;
    fill_extra_info(extra_info, nullptr, ctx, basename(file), fn, line);

    for(auto it = pointers_.begin(); it != pointers_.end(); ++it)
        do_show(it->first, pointers_, it, extra_info.data());
}

PointerLog &PointerLog::get_singleton()
{
    static PointerLog pointer_log_singleton;
    return pointer_log_singleton;
}
