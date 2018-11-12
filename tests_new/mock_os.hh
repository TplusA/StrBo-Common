/*
 * Copyright (C) 2018  T+A elektroakustik GmbH & Co. KG
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

#ifndef MOCK_OS_HH
#define MOCK_OS_HH

#include "os.h"
#include "mock_expectation.hh"

#include <string>
#include <functional>

namespace MockOS
{

/*! Base class for expectations. */
class Expectation
{
  public:
    Expectation(const Expectation &) = delete;
    Expectation(Expectation &&) = default;
    Expectation &operator=(const Expectation &) = delete;
    Expectation &operator=(Expectation &&) = default;
    Expectation() {}
    virtual ~Expectation() {}
};

class Mock
{
  private:
    bool suppress_errors_;

    MockExpectationsTemplate<Expectation> expectations_;

  public:
    Mock(const Mock &) = delete;
    Mock &operator=(const Mock &) = delete;

    explicit Mock():
        suppress_errors_(false),
        expectations_("MockOS")
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
        expectations_.add(std::move(expectation));
    }

    void expect(Expectation *expectation)
    {
        expectations_.add(std::unique_ptr<Expectation>(expectation));
    }

    void done() const { expectations_.done(); }

    template <typename T, typename ... Args>
    typename T::CheckReturnType check_next(Args ... args)
    {
        return expectations_.check_and_advance<T>(args...);
    }

    template <typename T>
    const T &next(const char *caller)
    {
        return expectations_.next<T>(caller);
    }
};

class Abort: public Expectation
{
  private:
    const int ret_errno_;

  public:
    explicit Abort(int ret_errno): ret_errno_(ret_errno) {}
    virtual ~Abort() {}
    using CheckReturnType = void;
    CheckReturnType check() const { errno = ret_errno_; }
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
        retval_(retval),
        ret_errno_(ret_errno),
        mapped_template_(mapped),
        arg_mapped_shall_be_null_(false),
        filename_(filename)
    {}

    explicit MapFileToMemory(int retval, int ret_errno,
                             bool expect_null_pointer, const char *filename):
        retval_(retval),
        ret_errno_(ret_errno),
        mapped_template_(nullptr),
        arg_mapped_shall_be_null_(expect_null_pointer),
        filename_(filename)
    {}

    using CheckReturnType = decltype(retval_);

    CheckReturnType check(struct os_mapped_file_data *mapped, const char *filename) const
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
        retval_(retval),
        ret_errno_(ret_errno),
        src_(src),
        arg_src_shall_be_null_(false),
        count_(count),
        fd_(fd)
    {}

    explicit WriteFromBuffer(int retval, int ret_errno,
                             bool expect_null_pointer, size_t count, int fd):
        retval_(retval),
        ret_errno_(ret_errno),
        src_(nullptr),
        arg_src_shall_be_null_(expect_null_pointer),
        count_(count),
        fd_(fd)
    {}

    using CheckReturnType = decltype(retval_);

    CheckReturnType check(const void *src, size_t count, int fd) const
    {
        if(callback_ != nullptr)
        {
            errno = ret_errno_;
            return callback_(src, count, fd);
        }

        if(arg_src_shall_be_null_)
            CHECK(src == nullptr);
        else
            CHECK(src != nullptr);

        CHECK(count == count_);
        CHECK(fd == fd_);

        errno = ret_errno_;

        return retval_;
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
        retval_(retval),
        ret_errno_(ret_errno),
        filename_(filename)
    {}

    using CheckReturnType = decltype(retval_);

    CheckReturnType check(const char *filename) const
    {
        REQUIRE(filename != nullptr);
        CHECK(filename == filename_);
        errno = ret_errno_;
        return retval_;
    }
};

class FileClose: public Expectation
{
  private:
    const int ret_errno_;
    const int fd_;

  public:
    explicit FileClose(int ret_errno, int fd):
        ret_errno_(ret_errno),
        fd_(fd)
    {}

    using CheckReturnType = void;

    CheckReturnType check(int fd) const
    {
        CHECK(fd == fd_);
        errno = ret_errno_;
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
        retval_(retval),
        ret_errno_(ret_errno),
        filename_(filename)
    {}

    using CheckReturnType = decltype(retval_);

    CheckReturnType check(const char *filename) const
    {
        REQUIRE(filename != nullptr);
        CHECK(filename == filename_);
        errno = ret_errno_;
        return retval_;
    }
};

class UnmapFile: public Expectation
{
  private:
    const int ret_errno_;
    const struct os_mapped_file_data *mapped_;

  public:
    explicit UnmapFile(int ret_errno, const struct os_mapped_file_data *mapped):
        ret_errno_(ret_errno),
        mapped_(mapped)
    {}

    using CheckReturnType = void;

    CheckReturnType check(struct os_mapped_file_data *mapped) const
    {
        CHECK(mapped != nullptr);
        CHECK(mapped == mapped_);
        errno = ret_errno_;
    }
};

extern Mock *singleton;

}

#endif /* !MOCK_OS_HH */
