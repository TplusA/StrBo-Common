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

#ifndef OS_H
#define OS_H

#include <stdbool.h>
#include <unistd.h>
#include <time.h>

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

int os_write_from_buffer(const void *src, size_t count, int fd);
int os_try_read_to_buffer(void *dest, size_t count, size_t *dest_pos, int fd);
void os_abort(void);

int os_system(const char *command);
int os_system_formatted(const char *format_string, ...)
    __attribute__ ((format (printf, 1, 2)));

bool os_foreach_in_path(const char *path,
                        void (*callback)(const char *path, void *user_data),
                        void *user_data);
enum os_path_type os_path_get_type(const char *path);

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

/*!
 * Flush changes in a directory to storage.
 */
void os_sync_dir(const char *path);

int os_map_file_to_memory(struct os_mapped_file_data *mapped,
                          const char *filename);
void os_unmap_file(struct os_mapped_file_data *mapped);

int os_clock_gettime(clockid_t clk_id, struct timespec *tp);
void os_nanosleep(const struct timespec *tp);

#ifdef __cplusplus
}
#endif

#endif /* !OS_H */
