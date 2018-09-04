/*
 * Copyright (C) 2015, 2017, 2018  T+A elektroakustik GmbH & Co. KG
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
#include <string>
#include <dirent.h>

#include "mock_os.hh"

enum class OsFn
{
    write_from_buffer,
    try_read_to_buffer,
    stdlib_abort,
    stdlib_system,
    system_formatted,
    foreach_in_path,
    path_get_type,
    path_get_number_of_hard_links,
    resolve_symlink,
    mkdir_hierarchy,
    unix_mkdir,
    unix_rmdir,
    file_new,
    file_close,
    file_delete,
    file_rename,
    link_new,
    sync_dir,
    map_file_to_memory,
    unmap_file,
    os_clock_gettime_fn,
    os_nanosleep_fn,
    os_sched_yield_fn,

    first_valid_os_fn_id = write_from_buffer,
    last_valid_os_fn_id = os_sched_yield_fn,
};


static std::ostream &operator<<(std::ostream &os, const OsFn id)
{
    if(id < OsFn::first_valid_os_fn_id ||
       id > OsFn::last_valid_os_fn_id)
    {
        os << "INVALID";
        return os;
    }

    switch(id)
    {
      case OsFn::write_from_buffer:
        os << "write_from_buffer";
        break;

      case OsFn::try_read_to_buffer:
        os << "try_read_to_buffer";
        break;

      case OsFn::stdlib_abort:
        os << "abort";
        break;

      case OsFn::stdlib_system:
        os << "stdlib_system";
        break;

      case OsFn::system_formatted:
        os << "system_formatted";
        break;

      case OsFn::foreach_in_path:
        os << "foreach_in_path";
        break;

      case OsFn::path_get_type:
        os << "path_get_type";
        break;

      case OsFn::path_get_number_of_hard_links:
        os << "path_get_number_of_hard_links";
        break;

      case OsFn::resolve_symlink:
        os << "resolve_symlink";
        break;

      case OsFn::mkdir_hierarchy:
        os << "mkdir_hierarchy";
        break;

      case OsFn::unix_mkdir:
        os << "unix_mkdir";
        break;

      case OsFn::unix_rmdir:
        os << "unix_rmdir";
        break;

      case OsFn::file_new:
        os << "file_new";
        break;

      case OsFn::file_close:
        os << "file_close";
        break;

      case OsFn::file_delete:
        os << "file_delete";
        break;

      case OsFn::file_rename:
        os << "file_rename";
        break;

      case OsFn::link_new:
        os << "link_new";
        break;

      case OsFn::sync_dir:
        os << "sync_dir";
        break;

      case OsFn::map_file_to_memory:
        os << "map_file_to_memory";
        break;

      case OsFn::unmap_file:
        os << "unmap_file";
        break;

      case OsFn::os_clock_gettime_fn:
        os << "os_clock_gettime";
        break;

      case OsFn::os_nanosleep_fn:
        os << "os_nanosleep";
        break;

      case OsFn::os_sched_yield_fn:
        os << "os_sched_yield";
        break;
    }

    os << "()";

    return os;
}

class MockOs::Expectation
{
  public:
    struct Data
    {
        const OsFn function_id_;
        const std::string ret_string_;
        const int ret_int_;
        const size_t ret_size_;
        const bool ret_bool_;
        enum os_path_type ret_path_type_;

        int ret_errno_;
        std::string arg_string_;
        std::string arg_string_second_;
        bool arg_bool_;
        int arg_fd_;
        const void *arg_src_pointer_;
        void *arg_dest_pointer_;
        struct os_mapped_file_data *arg_mapped_pointer_;
        const struct os_mapped_file_data *arg_mapped_template_;
        bool arg_pointer_expect_concrete_value_;
        bool arg_pointer_shall_be_null_;
        size_t arg_count_;
        size_t *arg_add_bytes_read_pointer_;
        WriteFromBufferCallback os_write_from_buffer_callback_;
        TryReadToBufferCallback os_try_read_to_buffer_callback_;
        std::function<void()> generic_callback_;
        clockid_t arg_clk_id_;
        struct timespec timespec_;
        ClockGettimeCallback os_clock_gettime_callback_;
        const std::vector<ForeachItemData> *foreach_item_data_;

      private:
        explicit Data(OsFn fn, int ret_int, size_t ret_size,
                      const char *ret_string, bool ret_bool,
                      enum os_path_type ret_path_type):
            function_id_(fn),
            ret_string_(ret_string != nullptr ? ret_string : ""),
            ret_int_(ret_int),
            ret_size_(ret_size),
            ret_bool_(ret_bool),
            ret_path_type_(ret_path_type),
            ret_errno_(ESRCH),
            arg_bool_(false),
            arg_fd_(-5),
            arg_src_pointer_(nullptr),
            arg_dest_pointer_(nullptr),
            arg_mapped_pointer_(nullptr),
            arg_mapped_template_(nullptr),
            arg_pointer_expect_concrete_value_(false),
            arg_pointer_shall_be_null_(false),
            arg_count_(0),
            arg_add_bytes_read_pointer_(nullptr),
            os_write_from_buffer_callback_(nullptr),
            os_try_read_to_buffer_callback_(nullptr),
            arg_clk_id_(CLOCK_REALTIME_COARSE),
            timespec_({0}),
            os_clock_gettime_callback_(nullptr),
            foreach_item_data_(nullptr)
        {}

      public:
        explicit Data(OsFn fn):
            Data(fn, -5, 123456, nullptr, false, OS_PATH_TYPE_OTHER)
        {}

        explicit Data(OsFn fn, int ret):
            Data(fn, ret, 123457, nullptr, false, OS_PATH_TYPE_OTHER)
        {}

        explicit Data(OsFn fn, size_t ret):
            Data(fn, -6, ret, nullptr, false, OS_PATH_TYPE_OTHER)
        {}

        explicit Data(OsFn fn, const char *ret):
            Data(fn, -7, 123458, ret, false, OS_PATH_TYPE_OTHER)
        {}

        explicit Data(OsFn fn, bool ret):
            Data(fn, -8, 123459, nullptr, ret, OS_PATH_TYPE_OTHER)
        {}

        explicit Data(OsFn fn, enum os_path_type ret):
            Data(fn, -9, 123460, nullptr, false, ret)
        {}
    };

    const Data d;

  private:
    /* writable reference for simple ctor code */
    Data &data_ = *const_cast<Data *>(&d);

  public:
    Expectation(const Expectation &) = delete;
    Expectation(Expectation &&) = default;
    Expectation &operator=(const Expectation &) = delete;

    explicit Expectation(OsFn fn):
        d(fn)
    {}

    template <typename T>
    explicit Expectation(OsFn fn, T ret):
        d(fn, ret)
    {}

    Expectation &expect_ret_errno(int errno_value)
    {
        data_.ret_errno_ = errno_value;
        return *this;
    }

    Expectation &expect_arg_source_pointer(const void *src)
    {
        data_.arg_src_pointer_ = src;
        data_.arg_pointer_expect_concrete_value_ = true;
        data_.arg_pointer_shall_be_null_ = (src == nullptr);
        data_.os_write_from_buffer_callback_ = nullptr;
        return *this;
    }

    Expectation &expect_arg_source_pointer_null(bool expect_null)
    {
        data_.arg_src_pointer_ = nullptr;
        data_.arg_pointer_expect_concrete_value_ = false;
        data_.arg_pointer_shall_be_null_ = expect_null;
        data_.os_write_from_buffer_callback_ = nullptr;
        return *this;
    }

    Expectation &set_write_buffer_callback(const WriteFromBufferCallback &fn)
    {
        data_.arg_src_pointer_ = nullptr;
        data_.arg_pointer_expect_concrete_value_ = false;
        data_.arg_pointer_shall_be_null_ = false;
        data_.os_write_from_buffer_callback_ = fn;
        return *this;
    }

    Expectation &expect_arg_dest_pointer(void *dest)
    {
        data_.arg_dest_pointer_ = dest;
        data_.arg_pointer_expect_concrete_value_ = true;
        data_.arg_pointer_shall_be_null_ = (dest == nullptr);
        data_.os_try_read_to_buffer_callback_ = nullptr;
        return *this;
    }

    Expectation &expect_arg_dest_pointer_null(bool expect_null)
    {
        data_.arg_dest_pointer_ = nullptr;
        data_.arg_pointer_expect_concrete_value_ = false;
        data_.arg_pointer_shall_be_null_ = expect_null;
        data_.os_try_read_to_buffer_callback_ = nullptr;
        return *this;
    }

    Expectation &set_try_read_buffer_callback(const TryReadToBufferCallback &fn)
    {
        data_.arg_dest_pointer_ = nullptr;
        data_.arg_pointer_expect_concrete_value_ = false;
        data_.arg_pointer_shall_be_null_ = false;
        data_.os_try_read_to_buffer_callback_ = fn;
        return *this;
    }

    Expectation &expect_arg_mapped_pointer(struct os_mapped_file_data *mapped)
    {
        data_.arg_mapped_pointer_ = mapped;
        data_.arg_mapped_template_ = nullptr;
        data_.arg_pointer_expect_concrete_value_ = false;
        data_.arg_pointer_shall_be_null_ = (mapped == nullptr);
        return *this;
    }

    Expectation &expect_arg_mapped_template(const struct os_mapped_file_data *mapped)
    {
        data_.arg_mapped_pointer_ = nullptr;
        data_.arg_mapped_template_ = mapped;
        data_.arg_pointer_expect_concrete_value_ = false;
        data_.arg_pointer_shall_be_null_ = false;
        return *this;
    }

    Expectation &expect_arg_mapped_null(bool expect_null)
    {
        data_.arg_mapped_pointer_ = nullptr;
        data_.arg_mapped_template_ = nullptr;
        data_.arg_pointer_expect_concrete_value_ = false;
        data_.arg_pointer_shall_be_null_ = expect_null;
        return *this;
    }

    Expectation &expect_arg_bool(bool flag)
    {
        data_.arg_bool_ = flag;
        return *this;
    }

    Expectation &expect_arg_string(const char *str)
    {
        data_.arg_string_ = str;
        return *this;
    }

    Expectation &expect_arg_string_second(const char *str)
    {
        data_.arg_string_second_ = str;
        return *this;
    }

    Expectation &expect_arg_count(size_t count)
    {
        data_.arg_count_ = count;
        return *this;
    }

    Expectation &expect_arg_fd(int fd)
    {
        data_.arg_fd_ = fd;
        return *this;
    }

    Expectation &expect_arg_add_bytes_read_pointer(size_t *add_bytes_read)
    {
        data_.arg_add_bytes_read_pointer_ = add_bytes_read;
        return *this;
    }

    Expectation &set_pointer_to_data_items(const std::vector<ForeachItemData> &items)
    {
        data_.foreach_item_data_ = &items;
        return *this;
    }

    Expectation &set_generic_callback(const std::function<void()> &cb)
    {
        data_.generic_callback_ = cb;
        return *this;
    }

    Expectation &expect_arg_clock_id(clockid_t clk_id)
    {
        data_.arg_clk_id_ = clk_id;
        return *this;
    }

    Expectation &expect_arg_timespec(const struct timespec &tspec)
    {
        data_.timespec_ = tspec;
        return *this;
    }

    Expectation &set_clock_gettime_callback(const ClockGettimeCallback &fn)
    {
        data_.os_clock_gettime_callback_ = fn;
        return *this;
    }
};

MockOs::MockOs():
    expectations_(new MockExpectations()),
    suppress_errors_(false)
{}

MockOs::~MockOs()
{
    delete expectations_;
}

void MockOs::init()
{
    cppcut_assert_not_null(expectations_);
    expectations_->init();
}

void MockOs::check() const
{
    cppcut_assert_not_null(expectations_);
    expectations_->check();
}

void MockOs::expect_os_write_from_buffer(int ret, const void *src, size_t count, int fd)
{
    expectations_->add(std::move(
        Expectation(OsFn::write_from_buffer, ret)
            .expect_arg_source_pointer(src)
            .expect_arg_count(count)
            .expect_arg_fd(fd)));
}

void MockOs::expect_os_write_from_buffer(int ret, bool expect_null_pointer, size_t count, int fd)
{
    expectations_->add(std::move(
        Expectation(OsFn::write_from_buffer, ret)
            .expect_arg_source_pointer_null(expect_null_pointer)
            .expect_arg_count(count)
            .expect_arg_fd(fd)));
}

void MockOs::expect_os_write_from_buffer_callback(const WriteFromBufferCallback &fn)
{
    expectations_->add(std::move(
        Expectation(OsFn::write_from_buffer).set_write_buffer_callback(fn)));
}

void MockOs::expect_os_try_read_to_buffer(int ret, void *dest, size_t count, size_t *add_bytes_read, int fd, bool suppress)
{
    expectations_->add(std::move(
        Expectation(OsFn::try_read_to_buffer, ret)
            .expect_arg_dest_pointer(dest)
            .expect_arg_count(count)
            .expect_arg_add_bytes_read_pointer(add_bytes_read)
            .expect_arg_fd(fd)
            .expect_arg_bool(suppress)));
}

void MockOs::expect_os_try_read_to_buffer(int ret, bool expect_null_pointer, size_t count,
                                          size_t *add_bytes_read, int fd, bool suppress)
{
    expectations_->add(std::move(
        Expectation(OsFn::try_read_to_buffer, ret)
            .expect_arg_dest_pointer_null(expect_null_pointer)
            .expect_arg_count(count)
            .expect_arg_add_bytes_read_pointer(add_bytes_read)
            .expect_arg_fd(fd)
            .expect_arg_bool(suppress)));
}

void MockOs::expect_os_try_read_to_buffer_callback(const TryReadToBufferCallback &fn)
{
    expectations_->add(std::move(
        Expectation(OsFn::try_read_to_buffer).set_try_read_buffer_callback(fn)));
}

void MockOs::expect_os_abort(void)
{
    expectations_->add(Expectation(OsFn::stdlib_abort));
}

void MockOs::expect_os_system(int retval, bool is_verbose, const char *command)
{
    expectations_->add(std::move(
        Expectation(OsFn::stdlib_system, retval)
            .expect_arg_bool(is_verbose)
            .expect_arg_string(command)));
}

void MockOs::expect_os_system_formatted(int retval, bool is_verbose, const char *string)
{
    expectations_->add(std::move(
        Expectation(OsFn::system_formatted, retval)
            .expect_arg_bool(is_verbose)
            .expect_arg_string(string)));
}

void MockOs::expect_os_foreach_in_path(int retval, const char *path)
{
    expectations_->add(std::move(
        Expectation(OsFn::foreach_in_path, retval).expect_arg_string(path)));
}

void MockOs::expect_os_foreach_in_path(int retval, const char *path,
                                       const std::vector<ForeachItemData> &items)
{
    expectations_->add(std::move(
        Expectation(OsFn::foreach_in_path, retval)
            .expect_arg_string(path)
            .set_pointer_to_data_items(items)));
}

void MockOs::expect_os_path_get_type(enum os_path_type retval, const char *path)
{
    expectations_->add(std::move(
        Expectation(OsFn::path_get_type, retval)
            .expect_arg_string(path)));
}

void MockOs::expect_os_path_get_number_of_hard_links(size_t retval, const char *path)
{
    expectations_->add(std::move(
        Expectation(OsFn::path_get_number_of_hard_links, retval)
            .expect_arg_string(path)));
}

void MockOs::expect_os_resolve_symlink(const char *retval, const char *link)
{
    expectations_->add(std::move(
        Expectation(OsFn::resolve_symlink, retval)
            .expect_arg_string(link)));
}

void MockOs::expect_os_mkdir_hierarchy(bool retval, const char *path, bool must_not_exist)
{
    expectations_->add(std::move(
        Expectation(OsFn::mkdir_hierarchy, retval)
            .expect_arg_string(path)
            .expect_arg_bool(must_not_exist)));
}

void MockOs::expect_os_mkdir(bool retval, const char *path, bool must_not_exist)
{
    expectations_->add(std::move(
        Expectation(OsFn::unix_mkdir, retval)
            .expect_arg_string(path)
            .expect_arg_bool(must_not_exist)));
}

void MockOs::expect_os_rmdir(bool retval, const char *path, bool must_exist)
{
    expectations_->add(std::move(
        Expectation(OsFn::unix_rmdir, retval)
            .expect_arg_string(path)
            .expect_arg_bool(must_exist)));
}

void MockOs::expect_os_file_new(int ret, const char *filename)
{
    expectations_->add(std::move(
        Expectation(OsFn::file_new, ret).expect_arg_string(filename)));
}

void MockOs::expect_os_file_close(int fd)
{
    expectations_->add(std::move(
        Expectation(OsFn::file_close).expect_arg_fd(fd)));
}

void MockOs::expect_os_file_delete(int ret, int ret_errno, const char *filename)
{
    expectations_->add(std::move(
        Expectation(OsFn::file_delete, ret)
            .expect_ret_errno(ret_errno)
            .expect_arg_string(filename)));
}

void MockOs::expect_os_file_rename(bool retval, const char *oldpath, const char *newpath)
{
    expectations_->add(std::move(
        Expectation(OsFn::file_rename, retval)
            .expect_arg_string(oldpath)
            .expect_arg_string_second(newpath)));
}

void MockOs::expect_os_link_new(bool retval, const char *oldpath, const char *newpath)
{
    expectations_->add(std::move(
        Expectation(OsFn::link_new, retval)
            .expect_arg_string(oldpath)
            .expect_arg_string_second(newpath)));
}

void MockOs::expect_os_sync_dir(const char *path)
{
    expectations_->add(std::move(
        Expectation(OsFn::sync_dir)
            .expect_arg_string(path)));
}

void MockOs::expect_os_sync_dir_callback(const char *path, const std::function<void()> &callback)
{
    expectations_->add(std::move(
        Expectation(OsFn::sync_dir)
            .expect_arg_string(path)
            .set_generic_callback(callback)));
}

void MockOs::expect_os_map_file_to_memory(int ret, struct os_mapped_file_data *mapped,
                                          const char *filename)
{
    expectations_->add(std::move(
        Expectation(OsFn::map_file_to_memory, ret)
            .expect_arg_mapped_pointer(mapped)
            .expect_arg_string(filename)));
}

void MockOs::expect_os_map_file_to_memory(int ret,
                                          const struct os_mapped_file_data *mapped,
                                          const char *filename)
{
    expectations_->add(std::move(
        Expectation(OsFn::map_file_to_memory, ret)
            .expect_arg_mapped_template(mapped)
            .expect_arg_string(filename)));
}

void MockOs::expect_os_map_file_to_memory(int ret, bool expect_null_pointer,
                                          const char *filename)
{
    expectations_->add(std::move(
        Expectation(OsFn::map_file_to_memory, ret)
            .expect_arg_mapped_null(expect_null_pointer)
            .expect_arg_string(filename)));
}

void MockOs::expect_os_unmap_file(struct os_mapped_file_data *mapped)
{
    expectations_->add(std::move(
        Expectation(OsFn::unmap_file)
            .expect_arg_mapped_pointer(mapped)));
}

void MockOs::expect_os_unmap_file(const struct os_mapped_file_data *mapped)
{
    expectations_->add(std::move(
        Expectation(OsFn::unmap_file)
            .expect_arg_mapped_template(mapped)));
}

void MockOs::expect_os_unmap_file(bool expect_null_pointer)
{
    expectations_->add(std::move(
        Expectation(OsFn::unmap_file)
            .expect_arg_mapped_null(expect_null_pointer)));
}

void MockOs::expect_os_clock_gettime(int ret, clockid_t clk_id,
                                     const struct timespec &ret_tp)
{
    expectations_->add(std::move(
        Expectation(OsFn::os_clock_gettime_fn, ret)
            .expect_arg_clock_id(clk_id)
            .expect_arg_timespec(ret_tp)));
}

void MockOs::expect_os_clock_gettime_callback(const ClockGettimeCallback &fn)
{
    expectations_->add(std::move(
        Expectation(OsFn::os_clock_gettime_fn)
            .set_clock_gettime_callback(fn)));
}

void MockOs::expect_os_nanosleep(const struct timespec *tp)
{
    expectations_->add(std::move(
        Expectation(OsFn::os_nanosleep_fn).expect_arg_timespec(*tp)));
}

void MockOs::expect_os_nanosleep(long milliseconds)
{
    const struct timespec tp =
    {
        .tv_sec = milliseconds / 1000L,
        .tv_nsec = 1000L * 1000L * (milliseconds % 1000L),
    };

    expectations_->add(std::move(
        Expectation(OsFn::os_nanosleep_fn).expect_arg_timespec(tp)));
}

void MockOs::expect_os_sched_yield(void)
{
    expectations_->add(Expectation(OsFn::os_sched_yield_fn));
}


MockOs *mock_os_singleton = nullptr;

bool os_suppress_error_messages(bool do_suppress)
{
    bool ret = mock_os_singleton->suppress_errors_;
    mock_os_singleton->suppress_errors_ = do_suppress;
    return ret;
}

int os_write_from_buffer(const void *src, size_t count, int fd)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::write_from_buffer);

    if(expect.d.os_write_from_buffer_callback_ != nullptr)
    {
        errno = expect.d.ret_errno_;
        return expect.d.os_write_from_buffer_callback_(src, count, fd);
    }

    if(expect.d.arg_pointer_expect_concrete_value_)
        cppcut_assert_equal(expect.d.arg_src_pointer_, src);
    else if(expect.d.arg_pointer_shall_be_null_)
        cppcut_assert_null(src);
    else
        cppcut_assert_not_null(src);

    cppcut_assert_equal(expect.d.arg_count_, count);
    cppcut_assert_equal(expect.d.arg_fd_, fd);

    errno = expect.d.ret_errno_;

    return expect.d.ret_int_;
}

int os_try_read_to_buffer(void *dest, size_t count, size_t *add_bytes_read, int fd, bool suppress_error_on_eagain)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::try_read_to_buffer);

    if(expect.d.os_try_read_to_buffer_callback_ != nullptr)
    {
        errno = expect.d.ret_errno_;
        return expect.d.os_try_read_to_buffer_callback_(dest, count, add_bytes_read, fd, suppress_error_on_eagain);
    }

    if(expect.d.arg_pointer_expect_concrete_value_)
        cppcut_assert_equal(expect.d.arg_dest_pointer_, dest);
    else if(expect.d.arg_pointer_shall_be_null_)
        cppcut_assert_null(dest);
    else
        cppcut_assert_not_null(dest);

    cppcut_assert_equal(expect.d.arg_count_, count);
    cppcut_assert_equal(expect.d.arg_add_bytes_read_pointer_, add_bytes_read);
    cppcut_assert_equal(expect.d.arg_fd_, fd);
    cppcut_assert_equal(expect.d.arg_bool_, suppress_error_on_eagain);

    errno = expect.d.ret_errno_;

    return expect.d.ret_int_;
}

void os_abort(void)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    errno = expect.d.ret_errno_;

    cppcut_assert_equal(expect.d.function_id_, OsFn::stdlib_abort);
}

int os_system(bool is_verbose, const char *command)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::stdlib_system);
    cppcut_assert_equal(expect.d.arg_bool_, is_verbose);
    cppcut_assert_equal(expect.d.arg_string_.c_str(), command);

    errno = expect.d.ret_errno_;

    return expect.d.ret_int_;
}

int os_system_formatted(bool is_verbose, const char *format_string, ...)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::stdlib_system);

    va_list va;
    char buffer[512];
    va_start(va, format_string);
    (void)vsnprintf(buffer, sizeof(buffer), format_string, va);
    va_end(va);

    cppcut_assert_equal(expect.d.arg_bool_, is_verbose);
    cppcut_assert_equal(expect.d.arg_string_.c_str(), buffer);

    errno = expect.d.ret_errno_;

    return expect.d.ret_int_;
}

int os_foreach_in_path(const char *path,
                       int (*callback)(const char *path, unsigned char dtype,
                                       void *user_data),
                       void *user_data)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::foreach_in_path);
    cppcut_assert_equal(expect.d.arg_string_.c_str(), path);
    cppcut_assert_not_null(reinterpret_cast<void *>(callback));

    if(expect.d.foreach_item_data_ != NULL)
    {
        int ret = 0;
        errno = ESRCH;

        for(const auto &item : *expect.d.foreach_item_data_)
        {
            if((ret = callback(item.item_name_.c_str(),
                               item.is_directory_ ? DT_DIR : DT_REG,
                               user_data)) != 0)
            {
                errno = EINTR;
                break;
            }
        }

        cppcut_assert_equal(expect.d.ret_errno_, errno);
        cppcut_assert_equal(expect.d.ret_int_, ret);
    }

    errno = expect.d.ret_errno_;

    return expect.d.ret_int_;
}

char *os_resolve_symlink(const char *link)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::resolve_symlink);
    cppcut_assert_not_null(link);
    cppcut_assert_equal(expect.d.arg_string_.c_str(), link);

    errno = expect.d.ret_errno_;

    return expect.d.ret_string_.empty() ? nullptr : strdup(expect.d.ret_string_.c_str());
}

bool os_mkdir_hierarchy(const char *path, bool must_not_exist)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::mkdir_hierarchy);
    cppcut_assert_equal(expect.d.arg_string_.c_str(), path);
    cppcut_assert_equal(expect.d.arg_bool_, must_not_exist);

    errno = expect.d.ret_errno_;

    return expect.d.ret_bool_;
}

bool os_mkdir(const char *path, bool must_not_exist)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::unix_mkdir);
    cppcut_assert_equal(expect.d.arg_string_.c_str(), path);
    cppcut_assert_equal(expect.d.arg_bool_, must_not_exist);

    errno = expect.d.ret_errno_;

    return expect.d.ret_bool_;
}

bool os_rmdir(const char *path, bool must_exist)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::unix_rmdir);
    cppcut_assert_equal(expect.d.arg_string_.c_str(), path);
    cppcut_assert_equal(expect.d.arg_bool_, must_exist);

    errno = expect.d.ret_errno_;

    return expect.d.ret_bool_;
}

enum os_path_type os_path_get_type(const char *path)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::path_get_type);
    cppcut_assert_equal(expect.d.arg_string_.c_str(), path);

    errno = expect.d.ret_errno_;

    return expect.d.ret_path_type_;
}

size_t os_path_get_number_of_hard_links(const char *path)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::path_get_number_of_hard_links);
    cppcut_assert_equal(expect.d.arg_string_.c_str(), path);

    errno = expect.d.ret_errno_;

    return expect.d.ret_size_;
}

int os_file_new(const char *filename)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::file_new);
    cppcut_assert_equal(expect.d.arg_string_, std::string(filename));

    errno = expect.d.ret_errno_;

    return expect.d.ret_int_;
}

void os_file_close(int fd)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::file_close);
    cppcut_assert_equal(expect.d.arg_fd_, fd);

    errno = expect.d.ret_errno_;
}

int os_file_delete(const char *filename)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::file_delete);
    cppcut_assert_equal(expect.d.arg_string_, std::string(filename));

    errno = expect.d.ret_errno_;

    return expect.d.ret_int_;
}

bool os_file_rename(const char *oldpath, const char *newpath)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::file_rename);
    cppcut_assert_equal(expect.d.arg_string_, std::string(oldpath));
    cppcut_assert_equal(expect.d.arg_string_second_, std::string(newpath));

    errno = expect.d.ret_errno_;

    return expect.d.ret_bool_;
}

bool os_link_new(const char *oldpath, const char *newpath)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::link_new);
    cppcut_assert_equal(expect.d.arg_string_, std::string(oldpath));
    cppcut_assert_equal(expect.d.arg_string_second_, std::string(newpath));

    errno = expect.d.ret_errno_;

    return expect.d.ret_bool_;
}

void os_sync_dir(const char *path)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::sync_dir);
    cppcut_assert_equal(expect.d.arg_string_, std::string(path));

    if(expect.d.generic_callback_ != nullptr)
        expect.d.generic_callback_();

    errno = expect.d.ret_errno_;
}

int os_map_file_to_memory(struct os_mapped_file_data *mapped,
                          const char *filename)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::map_file_to_memory);

    if(expect.d.arg_mapped_template_ != nullptr)
        *mapped = *expect.d.arg_mapped_template_;
    else
    {
        if(expect.d.arg_pointer_expect_concrete_value_)
            cppcut_assert_equal(expect.d.arg_mapped_pointer_, mapped);
        else if(expect.d.arg_pointer_shall_be_null_)
            cppcut_assert_null(mapped);
        else
            cppcut_assert_not_null(mapped);
    }

    cppcut_assert_equal(expect.d.arg_string_, std::string(filename));

    errno = expect.d.ret_errno_;

    return expect.d.ret_int_;
}

void os_unmap_file(struct os_mapped_file_data *mapped)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::unmap_file);

    if(expect.d.arg_mapped_template_ != nullptr)
        *mapped = *expect.d.arg_mapped_template_;
    else
    {
        if(expect.d.arg_pointer_expect_concrete_value_)
            cppcut_assert_equal(expect.d.arg_mapped_pointer_, mapped);
        else if(expect.d.arg_pointer_shall_be_null_)
            cppcut_assert_null(mapped);
        else
            cppcut_assert_not_null(mapped);
    }

    errno = expect.d.ret_errno_;
}

int os_clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::os_clock_gettime_fn);

    if(expect.d.os_clock_gettime_callback_ != nullptr)
    {
        errno = expect.d.ret_errno_;
        return expect.d.os_clock_gettime_callback_(clk_id, tp);
    }

    cppcut_assert_equal(expect.d.arg_clk_id_, clk_id);
    *tp = expect.d.timespec_;

    errno = expect.d.ret_errno_;

    return expect.d.ret_int_;
}

void os_nanosleep(const struct timespec *tp)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::os_nanosleep_fn);

    cppcut_assert_equal(expect.d.timespec_.tv_sec, tp->tv_sec);
    cppcut_assert_equal(expect.d.timespec_.tv_nsec, tp->tv_nsec);

    errno = expect.d.ret_errno_;
}

void os_sched_yield(void)
{
    const auto &expect(mock_os_singleton->expectations_->get_next_expectation(__func__));

    cppcut_assert_equal(expect.d.function_id_, OsFn::os_sched_yield_fn);
}
