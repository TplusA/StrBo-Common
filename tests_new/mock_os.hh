/*
 * Copyright (C) 2018--2020, 2022, 2023  T+A elektroakustik GmbH & Co. KG
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

#ifndef MOCK_OS_HH
#define MOCK_OS_HH

#include "os.h"
#include "mock_expectation.hh"

#include <string>
#include <cstring>
#include <functional>

namespace MockOS
{

/*! Base class for MockOS expectations. */
class Expectation: public MockExpectationBase
{
  public:
    Expectation(std::string &&name): MockExpectationBase(std::move(name)) {}
    virtual ~Expectation() {}
};

class Mock: public MockBase
{
  private:
    bool suppress_errors_;

  public:
    Mock(const Mock &) = delete;
    Mock &operator=(const Mock &) = delete;

    explicit Mock(std::shared_ptr<MockExpectationSequence> eseq = nullptr):
        MockBase("MockOS", eseq),
        suppress_errors_(false)
    {}

    ~Mock() {}

    bool suppress_error_messages(bool do_suppress)
    {
        bool temp = suppress_errors_;
        suppress_errors_ = do_suppress;
        return temp;
    }

    void expect(std::unique_ptr<Expectation> expectation)
    {
        add(std::move(expectation));
    }

    void expect(Expectation *expectation)
    {
        add(std::unique_ptr<Expectation>(expectation));
    }

    template <typename T, typename ... Args>
    auto &expect(Args ... args)
    {
        static_assert(std::is_base_of_v<Expectation, T> == true);
        return *static_cast<T *>(add(std::make_unique<T>(std::forward<Args>(args)...)));
    }

    template <typename T>
    void ignore(std::unique_ptr<Expectation> default_result)
    {
        ignore<T>(std::move(default_result));
    }

    template <typename T>
    void ignore(Expectation *default_result)
    {
        ignore<T>(std::unique_ptr<Expectation>(default_result));
    }
};

class Abort: public Expectation
{
  private:
    const int ret_errno_;

  public:
    explicit Abort(int ret_errno): Expectation("Abort"), ret_errno_(ret_errno) {}
    virtual ~Abort() {}
    void check() const { errno = ret_errno_; }

    static auto make_from_check_parameters()
    {
        return std::make_unique<Abort>(0);
    }
};

class ResolveSymlink: public Expectation
{
  private:
    const std::string retval_;
    const int ret_errno_;
    const std::string link_;

  public:
    explicit ResolveSymlink(const char *retval, int ret_errno, const char *link):
        Expectation("ResolveSymlink"),
        retval_(retval),
        ret_errno_(ret_errno),
        link_(link)
    {}

    char *check(const char *link) const
    {
        REQUIRE(link != nullptr);
        CHECK(link == link_);
        errno = ret_errno_;
        return retval_.empty() ? nullptr : strdup(retval_.c_str());
    }

    static auto make_from_check_parameters(const char *link)
    {
        return std::make_unique<ResolveSymlink>("/path/to/link_target", 0, link);
    }
};

class MapFileToMemory: public Expectation
{
  private:
    const int retval_;
    const int ret_errno_;
    const struct os_mapped_file_data *mapped_template_;
    const bool arg_mapped_shall_be_null_;
    const std::string filename_;

  public:
    explicit MapFileToMemory(int retval, int ret_errno,
                             const struct os_mapped_file_data *mapped,
                             const char *filename):
        Expectation("MapFileToMemory"),
        retval_(retval),
        ret_errno_(ret_errno),
        mapped_template_(mapped),
        arg_mapped_shall_be_null_(false),
        filename_(filename)
    {}

    explicit MapFileToMemory(int retval, int ret_errno,
                             bool expect_null_pointer, const char *filename):
        Expectation("MapFileToMemory"),
        retval_(retval),
        ret_errno_(ret_errno),
        mapped_template_(nullptr),
        arg_mapped_shall_be_null_(expect_null_pointer),
        filename_(filename)
    {}

    int check(struct os_mapped_file_data *mapped, const char *filename) const
    {
        if(mapped_template_ != nullptr)
        {
            REQUIRE(mapped != nullptr);
            *mapped = *mapped_template_;
        }
        else
        {
            if(arg_mapped_shall_be_null_)
                CHECK(mapped == nullptr);
            else
                CHECK(mapped != nullptr);
        }

        REQUIRE(filename != nullptr);
        CHECK(filename == filename_);

        errno = ret_errno_;

        return retval_;
    }

    static auto make_from_check_parameters(struct os_mapped_file_data *mapped, const char *filename)
    {
        return std::make_unique<MapFileToMemory>(0, 0, mapped, filename);
    }
};

class WriteFromBuffer: public Expectation
{
  public:
    using Callback = std::function<int(const void *src, size_t count, int fd)>;

  private:
    const int retval_;
    const int ret_errno_;
    const void *const src_;
    const bool arg_src_shall_be_null_;
    const size_t count_;
    const int fd_;
    const Callback callback_;

  public:
    explicit WriteFromBuffer(int ret_errno, const Callback &fn):
        Expectation("WriteFromBuffer"),
        retval_(-5),
        ret_errno_(ret_errno),
        src_(nullptr),
        arg_src_shall_be_null_(false),
        count_(SIZE_MAX),
        fd_(-91),
        callback_(fn)
    {}

    explicit WriteFromBuffer(int retval, int ret_errno,
                             const void *src, size_t count, int fd):
        Expectation("WriteFromBuffer"),
        retval_(retval),
        ret_errno_(ret_errno),
        src_(src),
        arg_src_shall_be_null_(false),
        count_(count),
        fd_(fd)
    {}

    explicit WriteFromBuffer(int retval, int ret_errno,
                             bool expect_null_pointer, size_t count, int fd):
        Expectation("WriteFromBuffer"),
        retval_(retval),
        ret_errno_(ret_errno),
        src_(nullptr),
        arg_src_shall_be_null_(expect_null_pointer),
        count_(count),
        fd_(fd)
    {}

    int check(const void *src, size_t count, int fd) const
    {
        if(callback_ != nullptr)
        {
            errno = ret_errno_;
            return callback_(src, count, fd);
        }

        if(arg_src_shall_be_null_)
            CHECK(src == nullptr);
        else
        {
            CHECK(src != nullptr);

            if(src_ != nullptr)
                CHECK(src == src_);
        }

        CHECK(count == count_);
        CHECK(fd == fd_);

        errno = ret_errno_;

        return retval_;
    }

    static auto make_from_check_parameters(const void *src, size_t count, int fd)
    {
        return std::make_unique<WriteFromBuffer>(0, 0, src, count, fd);
    }
};

class PathGetType: public Expectation
{
  private:
    const enum os_path_type retval_;
    const int ret_errno_;
    const std::string pathname_;

  public:
    explicit PathGetType(enum os_path_type retval, int ret_errno, const char *pathname):
        Expectation("PathGetType"),
        retval_(retval),
        ret_errno_(ret_errno),
        pathname_(pathname)
    {}

    enum os_path_type check(const char *pathname) const
    {
        REQUIRE(pathname != nullptr);
        CHECK(pathname == pathname_);
        errno = ret_errno_;
        return retval_;
    }

    static auto make_from_check_parameters(const char *pathname)
    {
        return std::make_unique<PathGetType>(OS_PATH_TYPE_OTHER, 0, pathname);
    }
};

class FileNew: public Expectation
{
  private:
    const int retval_;
    const int ret_errno_;
    const std::string filename_;

  public:
    explicit FileNew(int retval, int ret_errno, const char *filename):
        Expectation("FileNew"),
        retval_(retval),
        ret_errno_(ret_errno),
        filename_(filename)
    {}

    int check(const char *filename) const
    {
        REQUIRE(filename != nullptr);
        CHECK(filename == filename_);
        errno = ret_errno_;
        return retval_;
    }

    static auto make_from_check_parameters(const char *filename)
    {
        return std::make_unique<FileNew>(0, 0, filename);
    }
};

class FileClose: public Expectation
{
  private:
    const int ret_errno_;
    const int fd_;

  public:
    explicit FileClose(int ret_errno, int fd):
        Expectation("FileClose"),
        ret_errno_(ret_errno),
        fd_(fd)
    {}

    void check(int fd) const
    {
        CHECK(fd == fd_);
        errno = ret_errno_;
    }

    static auto make_from_check_parameters(int fd)
    {
        return std::make_unique<FileClose>(0, fd);
    }
};

class FileDelete: public Expectation
{
  private:
    const int retval_;
    const int ret_errno_;
    const std::string filename_;

  public:
    explicit FileDelete(int retval, int ret_errno, const char *filename):
        Expectation("FileDelete"),
        retval_(retval),
        ret_errno_(ret_errno),
        filename_(filename)
    {}

    int check(const char *filename) const
    {
        REQUIRE(filename != nullptr);
        CHECK(filename == filename_);
        errno = ret_errno_;
        return retval_;
    }

    static auto make_from_check_parameters(const char *filename)
    {
        return std::make_unique<FileDelete>(0, 0, filename);
    }
};

class UnmapFile: public Expectation
{
  private:
    const int ret_errno_;
    const struct os_mapped_file_data *mapped_;

  public:
    explicit UnmapFile(int ret_errno, const struct os_mapped_file_data *mapped):
        Expectation("UnmapFile"),
        ret_errno_(ret_errno),
        mapped_(mapped)
    {}

    void check(struct os_mapped_file_data *mapped) const
    {
        CHECK(mapped != nullptr);
        CHECK(mapped == mapped_);
        errno = ret_errno_;
    }

    static auto make_from_check_parameters(struct os_mapped_file_data *mapped)
    {
        return std::make_unique<UnmapFile>(0, mapped);
    }
};

extern Mock *singleton;

}

#endif /* !MOCK_OS_HH */
