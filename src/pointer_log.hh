/*
 * Copyright (C) 2022, 2023  T+A elektroakustik GmbH & Co. KG
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

#ifndef POINTER_LOG_HH
#define POINTER_LOG_HH

#include <string>
#include <memory>
#include <tuple>
#include <map>

/*!
 * Store and retrieve information about a pointer.
 *
 * This class is not meant for production code. Its purpose is to help writing
 * traces for pointers to the log in order to track down certain problems.
 *
 * The most convenient use of this class is via the macros defined in this
 * header file, such as #POINTER_LOG_ADD().
 */
class PointerLog
{
  public:
    using MapType =
        std::map<const void *, std::tuple<std::string, std::string, std::string, int>>;

  private:
    MapType pointers_;

  public:
    PointerLog(const PointerLog &) = delete;
    PointerLog(PointerLog &&) = delete;
    PointerLog &operator=(const PointerLog &) = delete;
    PointerLog &operator=(PointerLog &&) = delete;
    explicit PointerLog() = default;

    static PointerLog &get_singleton();

    template <typename P>
    void add(const P *ptr, const char *name, const char *ctx,
             const char *file, const char *fn, int line)
    {
        add_untyped(static_cast<const void *>(ptr), name, ctx, file, fn, line);
    }

    template <typename P>
    void add(const std::shared_ptr<P> &ptr, const char *name, const char *ctx,
             const char *file, const char *fn, int line)
    {
        add(ptr.get(), name, ctx, file, fn, line);
    }

    template <typename P>
    void add(const std::unique_ptr<P> &ptr, const char *name, const char *ctx,
             const char *file, const char *fn, int line)
    {
        add(ptr.get(), name, ctx, file, fn, line);
    }

    template <typename P>
    void remove(const P *ptr, const char *ctx, const char *file, const char *fn, int line)
    {
        remove_untyped(static_cast<const void *>(ptr), ctx, file, fn, line);
    }

    template <typename P>
    void remove(const std::shared_ptr<P> &ptr, const char *ctx,
                const char *file, const char *fn, int line)
    {
        remove(ptr.get(), ctx, file, fn, line);
    }

    template <typename P>
    void remove(const std::unique_ptr<P> &ptr, const char *ctx,
                const char *file, const char *fn, int line)
    {
        remove(ptr.get(), ctx, file, fn, line);
    }

    template <typename P>
    void show(const P *ptr, const char *ctx = nullptr,
              const char *file = nullptr, const char *fn = nullptr, int line = -1)
    {
        show_untyped(static_cast<const void *>(ptr), ctx, file, fn, line);
    }

    template <typename P>
    void show(const std::shared_ptr<P> &ptr, const char *ctx = nullptr,
              const char *file = nullptr, const char *fn = nullptr, int line = -1)
    {
        show(ptr.get(), ctx, file, fn, line);
    }

    template <typename P>
    void show(const std::unique_ptr<P> &ptr, const char *ctx = nullptr,
              const char *file = nullptr, const char *fn = nullptr, int line = -1)
    {
        show(ptr.get(), ctx, file, fn, line);
    }

    void show_all(const char *ctx = nullptr,
                  const char *file = nullptr, const char *fn = nullptr, int line = -1);

  private:
    void add_untyped(const void *vp, const char *name, const char *ctx,
                     const char *file, const char *fn, int line);
    void remove_untyped(const void *vp, const char *ctx,
                        const char *file, const char *fn, int line);
    void show_untyped(const void *vp, const char *ctx,
                      const char *file, const char *fn, int line);
};

#define POINTER_LOG_ADD(PTR, NAME) \
    PointerLog::get_singleton().add((PTR), (NAME), nullptr, __FILE__, __func__, __LINE__)

#define POINTER_LOG_ADD_WITH_CTX(PTR, NAME, CTX) \
    PointerLog::get_singleton().add((PTR), (NAME), (CTX), __FILE__, __func__, __LINE__)

#define POINTER_LOG_REMOVE(PTR) \
    PointerLog::get_singleton().remove((PTR), nullptr, __FILE__, __func__, __LINE__)

#define POINTER_LOG_REMOVE_WITH_CTX(PTR, CTX) \
    PointerLog::get_singleton().remove((PTR), (CTX), __FILE__, __func__, __LINE__)

#define POINTER_LOG_SHOW(PTR) \
    PointerLog::get_singleton().show((PTR), nullptr, __FILE__, __func__, __LINE__)

#define POINTER_LOG_SHOW_WITH_CTX(PTR, CTX) \
    PointerLog::get_singleton().show((PTR), (CTX), __FILE__, __func__, __LINE__)

#endif /* !POINTER_LOG_HH */
