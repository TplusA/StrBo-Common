/*
 * Copyright (C) 2015, 2017, 2018, 2019  T+A elektroakustik GmbH & Co. KG
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

#include <functional>

#include "os.h"
#include "mock_expectation.hh"

class MockOs
{
  public:
    class ForeachItemData
    {
      public:
        const std::string item_name_;
        const bool is_directory_;

        ForeachItemData(const ForeachItemData &) = delete;
        ForeachItemData(ForeachItemData &&) = default;
        ForeachItemData &operator=(const ForeachItemData &) = delete;

        explicit ForeachItemData(std::string &&item_name, bool is_directory = false):
            item_name_(item_name),
            is_directory_(is_directory)
        {}
    };

    MockOs(const MockOs &) = delete;
    MockOs &operator=(const MockOs &) = delete;

    class Expectation;
    typedef MockExpectationsTemplate<Expectation> MockExpectations;
    MockExpectations *expectations_;

    bool suppress_errors_;

    explicit MockOs();
    ~MockOs();

    void init();
    void check() const;

    using WriteFromBufferCallback = std::function<int(const void *src, size_t count, int fd)>;
    void expect_os_write_from_buffer(int ret, int ret_errno, const void *src, size_t count, int fd);
    void expect_os_write_from_buffer(int ret, int ret_errno, bool expect_null_pointer, size_t count, int fd);
    void expect_os_write_from_buffer_callback(int ret_errno, const WriteFromBufferCallback &fn);

    using TryReadToBufferCallback =
        std::function<int(void *dest, size_t count, size_t *add_bytes_read, int fd,
                          bool suppress_error_on_eagain)>;
    void expect_os_try_read_to_buffer(int ret, int ret_errno, void *dest, size_t count,
                                      size_t *add_bytes_read, int fd, bool suppress);
    void expect_os_try_read_to_buffer(int ret, int ret_errno, bool expect_null_pointer, size_t count,
                                      size_t *add_bytes_read, int fd, bool suppress);
    void expect_os_try_read_to_buffer_callback(int ret_errno, const TryReadToBufferCallback &fn);

    void expect_os_abort(int ret_errno);
    void expect_os_system(int retval, int ret_errno, bool is_verbose, const char *command);
    void expect_os_system_formatted(int retval, int ret_errno, bool is_verbose, const char *string);
    void expect_os_foreach_in_path(int retval, int ret_errno, const char *path);
    void expect_os_foreach_in_path(int retval, int ret_errno, const char *path,
                                   const std::vector<ForeachItemData> &items);
    void expect_os_path_get_type(enum os_path_type retval, int ret_errno, const char *path);
    void expect_os_path_get_number_of_hard_links(size_t retval, int ret_errno, const char *path);
    void expect_os_resolve_symlink(const char *retval, int ret_errno, const char *link);
    void expect_os_mkdir_hierarchy(bool retval, int ret_errno, const char *path, bool must_not_exist);
    void expect_os_mkdir(bool retval, int ret_errno, const char *path, bool must_not_exist);
    void expect_os_rmdir(bool retval, int ret_errno, const char *path, bool must_exist);
    void expect_os_file_new(int ret, int ret_errno, const char *filename);
    void expect_os_file_close(int ret_errno, int fd);
    void expect_os_file_delete(int ret, int ret_errno, const char *filename);
    void expect_os_file_rename(bool retval, int ret_errno, const char *oldpath, const char *newpath);
    void expect_os_link_new(bool retval, int ret_errno, const char *oldpath, const char *newpath);
    void expect_os_sync_dir(int ret_errno, const char *path);
    void expect_os_sync_dir_callback(int ret_errno, const char *path, const std::function<void()> &callback);
    void expect_os_map_file_to_memory(int ret, int ret_errno, struct os_mapped_file_data *mapped,
                                      const char *filename);
    void expect_os_map_file_to_memory(int ret, int ret_errno, const struct os_mapped_file_data *mapped,
                                      const char *filename);
    void expect_os_map_file_to_memory(int ret, int ret_errno, bool expect_null_pointer,
                                      const char *filename);
    void expect_os_unmap_file(int ret_errno, struct os_mapped_file_data *mapped);
    void expect_os_unmap_file(int ret_errno, const struct os_mapped_file_data *mapped);
    void expect_os_unmap_file(int ret_errno, bool expect_null_pointer);

    using ClockGettimeCallback = std::function<int(clockid_t clk_id, struct timespec *tp)>;
    void expect_os_clock_gettime(int ret, int ret_errno, clockid_t clk_id, const struct timespec &ret_tp);
    void expect_os_clock_gettime_callback(int ret_errno, const ClockGettimeCallback &fn);

    void expect_os_nanosleep(int ret_errno, const struct timespec *tp);
    void expect_os_nanosleep(int ret_errno, long milliseconds);

    void expect_os_sched_yield();
};

extern MockOs *mock_os_singleton;

#endif /* !MOCK_OS_HH */
