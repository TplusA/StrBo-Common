/*
 * Copyright (C) 2015, 2017  T+A elektroakustik GmbH & Co. KG
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

#ifndef OS_H
#define OS_H

#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/time.h>

/*!
 * Data for keeping track of memory-mapped files.
 */
struct os_mapped_file_data
{
    int fd;
    void *ptr;
    size_t length;
};

enum os_path_type
{
    OS_PATH_TYPE_IO_ERROR,
    OS_PATH_TYPE_DIRECTORY,
    OS_PATH_TYPE_FILE,
    OS_PATH_TYPE_OTHER,
};

#ifdef __cplusplus
extern "C" {
#endif

extern ssize_t (*os_read)(int fd, void *dest, size_t count);
extern ssize_t (*os_write)(int fd, const void *buf, size_t count);
extern int (*os_poll)(struct pollfd *fds, nfds_t nfds, int timeout);

bool os_suppress_error_messages(bool do_suppress);

int os_write_from_buffer(const void *src, size_t count, int fd);
int os_try_read_to_buffer(void *dest, size_t count, size_t *dest_pos, int fd,
                          bool suppress_error_on_eagain);
void os_abort(void);

int os_system(bool is_verbose, const char *command);
int os_system_formatted(bool is_verbose, const char *format_string, ...)
    __attribute__ ((format (printf, 2, 3)));

/*!
 * Read directory, call callback for each item.
 *
 * \param path
 *     Which directory to read.
 *
 * \param callback
 *     A function that is called for each item in the given directory. The
 *     function must return 0 to continue reading more directory entries. In
 *     case the function returns a non-zero value, the iteration over the
 *     directory is stopped, and the return value becomes the return value of
 *     #os_foreach_in_path(). Implementers of the callback function are advised
 *     to return positive values in non-error situations, easily allowing the
 *     caller to tell errors from deliberate, non-erroneous interruptions.
 *
 * \param user_data
 *     Data passed unmodified to \p callback.
 *
 * \returns
 *     0 on success, a negative value in case of error (check errno), or the
 *     non-zero return value of \p callback.
 *
 */
int os_foreach_in_path(const char *path,
                       int (*callback)(const char *path, unsigned char dtype,
                                       void *user_data),
                       void *user_data);

enum os_path_type os_path_get_type(const char *path);
size_t os_path_get_number_of_hard_links(const char *path);
bool os_path_utimes(const char *path, const struct timeval *times);

/*!
 * Plain wrapper around lstat(2), no fanciness here.
 */
int os_lstat(const char *path, struct stat *buf);

/*!
 * Plain wrapper around stat(2), no fanciness here.
 */
int os_stat(const char *path, struct stat *buf);

/*!
 * Read destination of symlink, if any.
 *
 * \returns
 *     A string to be freed by the caller, or \c NULL if the input name is not
 *     a symlink, the symlink is broken, or any kind of error is returned from
 *     the OS.
 */
char *os_resolve_symlink(const char *link);

bool os_mkdir_hierarchy(const char *path, bool must_not_exist);
bool os_mkdir(const char *path, bool must_not_exist);
bool os_rmdir(const char *path, bool must_exist);

int os_file_new(const char *filename);
void os_file_close(int fd);
void os_file_delete(const char *filename);
bool os_file_rename(const char *oldpath, const char *newpath);

bool os_link_new(const char *oldpath, const char *newpath);

/*!
 * Flush changes in a directory to storage.
 */
void os_sync_dir(const char *path);

int os_map_file_to_memory(struct os_mapped_file_data *mapped,
                          const char *filename);
void os_unmap_file(struct os_mapped_file_data *mapped);

int os_clock_gettime(clockid_t clk_id, struct timespec *tp);
void os_nanosleep(const struct timespec *tp);
void os_sched_yield(void);

#ifdef __cplusplus
}
#endif

#endif /* !OS_H */
