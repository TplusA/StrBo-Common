/*
 * Copyright (C) 2015  T+A elektroakustik GmbH & Co. KG
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

#include <functional>

#include "os.h"
#include "mock_expectation.hh"

class MockOs
{
  public:
    MockOs(const MockOs &) = delete;
    MockOs &operator=(const MockOs &) = delete;

    class Expectation;
    typedef MockExpectationsTemplate<Expectation> MockExpectations;
    MockExpectations *expectations_;

    explicit MockOs();
    ~MockOs();

    void init();
    void check() const;

    typedef int (*os_write_from_buffer_callback_t)(const void *src, size_t count, int fd);
    void expect_os_write_from_buffer(int ret, const void *src, size_t count, int fd);
    void expect_os_write_from_buffer(int ret, bool expect_null_pointer, size_t count, int fd);
    void expect_os_write_from_buffer_callback(os_write_from_buffer_callback_t fn);

    typedef int (*os_try_read_to_buffer_callback_t)(void *dest, size_t count, size_t *add_bytes_read, int fd, bool suppress_error_on_eagain);
    void expect_os_try_read_to_buffer(int ret, void *dest, size_t count,
                                      size_t *add_bytes_read, int fd, bool suppress);
    void expect_os_try_read_to_buffer(int ret, bool expect_null_pointer, size_t count,
                                      size_t *add_bytes_read, int fd, bool suppress);
    void expect_os_try_read_to_buffer_callback(os_try_read_to_buffer_callback_t fn);

    void expect_os_abort(void);
    void expect_os_system(int retval, const char *command);
    void expect_os_system_formatted(int retval, const char *string);
    void expect_os_system_formatted_formatted(int retval, const char *string);
    void expect_os_foreach_in_path(bool retval, const char *path);
    void expect_os_path_get_type(enum os_path_type retval, const char *path);
    void expect_os_resolve_symlink(const char *retval, const char *link);
    void expect_os_mkdir_hierarchy(bool retval, const char *path, bool must_not_exist);
    void expect_os_mkdir(bool retval, const char *path, bool must_not_exist);
    void expect_os_rmdir(bool retval, const char *path, bool must_exist);
    void expect_os_file_new(int ret, const char *filename);
    void expect_os_file_close(int fd);
    void expect_os_file_delete(const char *filename);
    void expect_os_sync_dir(const char *path);
    void expect_os_sync_dir_callback(const char *path, const std::function<void()> &callback);
    void expect_os_map_file_to_memory(int ret, struct os_mapped_file_data *mapped,
                                      const char *filename);
    void expect_os_map_file_to_memory(const struct os_mapped_file_data *mapped,
                                      const char *filename);
    void expect_os_map_file_to_memory(int ret, bool expect_null_pointer,
                                      const char *filename);
    void expect_os_unmap_file(struct os_mapped_file_data *mapped);
    void expect_os_unmap_file(const struct os_mapped_file_data *mapped);
    void expect_os_unmap_file(bool expect_null_pointer);

    typedef int (*os_clock_gettime_callback_t)(clockid_t clk_id, struct timespec *tp);
    void expect_os_clock_gettime(int ret, clockid_t clk_id, const struct timespec &ret_tp);
    void expect_os_clock_gettime_callback(os_clock_gettime_callback_t fn);

    void expect_os_nanosleep(const struct timespec *tp);
    void expect_os_nanosleep(long milliseconds);
};

extern MockOs *mock_os_singleton;

#endif /* !MOCK_OS_HH */
