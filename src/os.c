/*
 * Copyright (C) 2015--2020, 2022, 2024  T+A elektroakustik GmbH & Co. KG
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

#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <linux/fs.h>
#include <limits.h>
#include <sched.h>

#include "os.h"
#include "messages.h"

#if MSG_ACTION_ON_ABORT == 1
#include "backtrace.h"
#endif

#define SAVE_ERRNO(VAR)         const int VAR = errno
#define RESTORE_ERRNO(VAR)      errno = VAR

static struct
{
    bool suppress_errors;
}
verbosity;

bool os_suppress_error_messages(bool do_suppress)
{
    bool ret = verbosity.suppress_errors;
    verbosity.suppress_errors = do_suppress;
    return ret;
}

int os_write_from_buffer(const void *src, size_t count, int fd)
{
    const uint8_t *src_ptr = src;

    errno = 0;

    while(count > 0)
    {
        ssize_t len;

        while((len = os_write(fd, src_ptr, count)) == -1 && errno == EINTR)
            ;

        if(len < 0)
        {
            if(!verbosity.suppress_errors)
            {
                SAVE_ERRNO(temp);
                msg_error(errno, LOG_ERR, "Failed writing to fd %d", fd);
                RESTORE_ERRNO(temp);
            }

            return -1;
        }

        msg_log_assert((size_t)len <= count);

        src_ptr += len;
        count -= len;
        errno = 0;
    }

    return 0;
}

int os_try_read_to_buffer(void *dest, size_t count, size_t *dest_pos, int fd,
                          bool suppress_error_on_eagain)
{
    uint8_t *dest_ptr = dest;

    dest_ptr += *dest_pos;
    count -= *dest_pos;
    errno = 0;

    int retval = 0;

    while(count > 0)
    {
        const ssize_t len = os_read(fd, dest_ptr, count);

        if(len == 0)
        {
            errno = 0;
            break;
        }

        if(len < 0)
        {
            retval = (errno == EAGAIN) ? 0 : -1;

            if(errno != EAGAIN || !suppress_error_on_eagain)
            {
                SAVE_ERRNO(temp);
                msg_error(errno, LOG_ERR, "Failed reading from fd %d", fd);
                RESTORE_ERRNO(temp);
            }

            break;
        }

        msg_log_assert((size_t)len <= count);

        dest_ptr += len;
        count -= len;
        *dest_pos += len;
        retval = 1;
        errno = 0;
    }

    return retval;
}

void os_abort(void)
{
#if MSG_ACTION_ON_ABORT == 1
    backtrace_log(0, "abort context");
#endif /* MSG_ACTION_ON_ABORT */

    abort();
}

int os_system(bool is_verbose, const char *command)
{
    if(is_verbose)
        msg_info("Executing external command: %s", command);

    const int ret = system(command);

    if(WIFEXITED(ret))
    {
        if(WEXITSTATUS(ret) == EXIT_SUCCESS)
        {
            if(is_verbose)
                msg_info("External command succeeded");
        }
        else if(!verbosity.suppress_errors)
            msg_error(0, LOG_ERR,
                      "External command failed with exit code %d",
                      WEXITSTATUS(ret));

        return WEXITSTATUS(ret);
    }

    if(WCOREDUMP(ret))
    {
        msg_error(0, LOG_ERR, "CRASHED: \"%s\"", command);
        return INT_MIN;
    }

    if(WIFSIGNALED(ret))
    {
        msg_error(0, LOG_ERR, "TERMINATED by signal %d: \"%s\"",
                  WTERMSIG(ret), command);
        return -WTERMSIG(ret);
    }

    MSG_BUG("Bogus exit code %d from external command", ret);

    return ret;
}

int os_system_formatted(bool is_verbose, const char *format_string, ...)
{
    char buffer[1024];

    va_list va;
    va_start(va, format_string);

    (void)vsnprintf(buffer, sizeof(buffer), format_string, va);

    va_end(va);

    return os_system(is_verbose, buffer);
}

static bool is_valid_directory_name(const char *path)
{
    if(path[0] != '.')
        return true;

    if(path[1] == '.')
        return path[2] != '\0';
    else
        return path[1] != '\0';
}

int os_foreach_in_path(const char *path,
                       int (*callback)(const char *path, unsigned char dtype,
                                       void *user_data),
                       void *user_data)
{
    msg_log_assert(path != NULL);
    msg_log_assert(callback != NULL);

    errno = 0;

    DIR *dir = opendir(path);

    if(dir == NULL)
    {
        if(!verbosity.suppress_errors)
        {
            SAVE_ERRNO(temp);
            msg_error(errno, LOG_ERR, "Failed opening directory \"%s\"", path);
            RESTORE_ERRNO(temp);
        }

        return -1;
    }

    int retval = 0;

    while(true)
    {
        errno = 0;
        struct dirent *result = readdir(dir);

        if(result != NULL)
        {
            if(is_valid_directory_name(result->d_name) &&
               (retval = callback(result->d_name, result->d_type, user_data)) != 0)
            {
                errno = EINTR;
                break;
            }
        }
        else
        {
            retval = (errno == 0) ? 0 : -2;

            if(retval < 0 && !verbosity.suppress_errors)
            {
                SAVE_ERRNO(temp);
                msg_error(errno, LOG_ERR,
                          "Failed reading directory \"%s\"", path);
                RESTORE_ERRNO(temp);
            }

            break;
        }
    }

    SAVE_ERRNO(temp);
    closedir(dir);
    RESTORE_ERRNO(temp);

    return retval;
}

enum os_path_type os_path_get_type(const char *path)
{
    struct stat buf;

    if(stat(path, &buf) < 0)
    {
        if(!verbosity.suppress_errors)
        {
            SAVE_ERRNO(temp);
            msg_error(errno, LOG_ERR, "Failed to stat() file \"%s\"", path);
            RESTORE_ERRNO(temp);
        }

        return OS_PATH_TYPE_IO_ERROR;
    }

    if(S_ISDIR(buf.st_mode))
        return OS_PATH_TYPE_DIRECTORY;

    if(S_ISREG(buf.st_mode))
        return OS_PATH_TYPE_FILE;

    return OS_PATH_TYPE_OTHER;
}

size_t os_path_get_number_of_hard_links(const char *path)
{
    struct stat buf;

    if(stat(path, &buf) < 0)
    {
        if(!verbosity.suppress_errors)
        {
            SAVE_ERRNO(temp);
            msg_error(errno, LOG_ERR, "Failed to stat() file \"%s\"", path);
            RESTORE_ERRNO(temp);
        }

        return 0;
    }

    return buf.st_nlink;
}

bool os_path_utimes(const char *path, const struct timeval *times)
{
    errno = 0;

    if(utimes(path, times) < 0)
    {
        if(!verbosity.suppress_errors)
        {
            SAVE_ERRNO(temp);
            msg_error(errno, LOG_ERR, "Failed setting timestamps on \"%s\"", path);
            RESTORE_ERRNO(temp);
        }

        return false;
    }

    return true;
}

int os_lstat(const char *path, struct stat *buf)
{
    const int ret = lstat(path, buf);

    if(ret < 0 && !verbosity.suppress_errors)
    {
        SAVE_ERRNO(temp);
        msg_error(errno, LOG_ERR, "Failed to lstat() file \"%s\"", path);
        RESTORE_ERRNO(temp);
    }

    return ret;
}

int os_stat(const char *path, struct stat *buf)
{
    const int ret = stat(path, buf);

    if(ret < 0 && !verbosity.suppress_errors)
    {
        SAVE_ERRNO(temp);
        msg_error(errno, LOG_ERR, "Failed to stat() file \"%s\"", path);
        RESTORE_ERRNO(temp);
    }

    return ret;
}

char *os_resolve_symlink(const char *link)
{
    msg_log_assert(link != NULL);

    char dummy;

    if(readlink(link, &dummy, sizeof(dummy)) < 0)
    {
        if(!verbosity.suppress_errors)
        {
            SAVE_ERRNO(temp);

            if(errno == EINVAL)
                msg_error(errno, LOG_NOTICE,
                          "Path \"%s\" is not a symlink", link);
            else
                msg_error(errno, LOG_NOTICE,
                          "readlink() failed for path \"%s\"", link);

            RESTORE_ERRNO(temp);
        }

        return NULL;
    }

    char *const result = realpath(link, NULL);

    if(result == NULL && !verbosity.suppress_errors)
    {
        SAVE_ERRNO(temp);
        msg_error(errno, LOG_NOTICE,
                  "Failed resolving symlink \"%s\"", link);
        RESTORE_ERRNO(temp);
    }

    return result;
}

bool os_mkdir_hierarchy(const char *path, bool must_not_exist, bool is_world_readable)
{
    static const char failed_err[] = "Failed creating directory hierarchy %s";

    msg_log_assert(path != NULL);

    errno = 0;

    if(must_not_exist)
    {
        struct stat buf;

        if(lstat(path, &buf) == 0)
        {
            if(!verbosity.suppress_errors)
                msg_error(EEXIST, LOG_ERR, failed_err, path);

            errno = EEXIST;
            return false;
        }
    }

    /* oh well... */
    if(os_system_formatted(false, "mkdir -m 075%c -p %s",
                           is_world_readable ? '5' : '0', path) == EXIT_SUCCESS)
        return true;

    struct stat buf;

    if(lstat(path, &buf) == 0)
    {
        /* that's just weird */
        if(S_ISDIR(buf.st_mode))
            return true;

        errno = ENOTDIR;
    }

    SAVE_ERRNO(temp);
    msg_error(errno, LOG_ERR, failed_err, path);
    RESTORE_ERRNO(temp);

    return false;
}

bool os_mkdir(const char *path, bool must_not_exist)
{
    msg_log_assert(path != NULL);

    errno = 0;

    if(mkdir(path, 0750) == 0)
        return true;

    SAVE_ERRNO(temp);

    if(errno == EEXIST && !must_not_exist)
    {
        /* better make sure this is really a directory */
        struct stat buf;

        if(lstat(path, &buf) == 0 && S_ISDIR(buf.st_mode))
            return true;
    }

    if(!verbosity.suppress_errors)
        msg_error(errno, LOG_ERR, "Failed creating directory %s", path);

    RESTORE_ERRNO(temp);

    return false;
}

bool os_rmdir(const char *path, bool must_exist)
{
    msg_log_assert(path != NULL);

    errno = 0;

    if(rmdir(path) == 0)
        return true;

    if(must_exist && !verbosity.suppress_errors)
    {
        SAVE_ERRNO(temp);
        msg_error(errno, LOG_ERR, "Failed removing directory %s", path);
        RESTORE_ERRNO(temp);
    }

    return false;
}

int os_file_new(const char *filename)
{
    errno = 0;

    int fd;

    while((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
                     S_IRWXU | S_IRWXG | S_IRWXO)) == -1 &&
          errno == EINTR)
        ;

    if(fd < 0 && !verbosity.suppress_errors)
    {
        SAVE_ERRNO(temp);
        msg_error(errno, LOG_ERR, "Failed to create file \"%s\"", filename);
        RESTORE_ERRNO(temp);
    }

    return fd;
}

static void safe_close_fd(int fd)
{
    SAVE_ERRNO(previous_errno);

    errno = 0;

    if(fsync(fd) < 0 && errno != EINVAL && !verbosity.suppress_errors)
        msg_error(errno, LOG_ERR, "fsync() fd %d", fd);

    int ret;
    while((ret = close(fd)) == -1 && errno == EINTR)
        ;

    if(ret == 0)
        RESTORE_ERRNO(previous_errno);
    else if(ret == -1 && errno != EINTR && !verbosity.suppress_errors)
    {
        SAVE_ERRNO(temp);
        msg_error(errno, LOG_ERR, "Failed to close file descriptor %d", fd);
        RESTORE_ERRNO(temp);
    }
}

void os_file_close(int fd)
{
    if(fd < 0)
    {
        msg_error(EBADF, LOG_ERR,
                  "Passed invalid file descriptor %d to %s()", fd, __func__);
        errno = EBADF;
    }
    else
        safe_close_fd(fd);
}

int os_file_delete(const char *filename)
{
    msg_log_assert(filename != NULL);

    errno = 0;
    const int ret = unlink(filename);
    if(ret < 0 && !verbosity.suppress_errors)
    {
        SAVE_ERRNO(temp);
        msg_error(errno, LOG_ERR, "Failed to delete file \"%s\"", filename);
        RESTORE_ERRNO(temp);
    }

    return ret;
}

bool os_file_rename(const char *oldpath, const char *newpath)
{
    errno = 0;

    int ret;

    while((ret = rename(oldpath, newpath)) == -1 && errno == EINTR)
        ;

    if(ret < 0 && !verbosity.suppress_errors)
    {
        SAVE_ERRNO(temp);
        msg_error(errno, LOG_ERR, "Failed to rename \"%s\" to \"%s\"",
                  oldpath, newpath);
        RESTORE_ERRNO(temp);
    }

    return ret == 0;
}

bool os_link_new(const char *oldpath, const char *newpath)
{
    errno = 0;

    int ret;

    while((ret = link(oldpath, newpath)) == -1 && errno == EINTR)
        ;

    if(ret < 0 && !verbosity.suppress_errors)
    {
        SAVE_ERRNO(temp);
        msg_error(errno, LOG_ERR,
                  "Failed to create link \"%s\" from source \"%s\"",
                  newpath, oldpath);
        RESTORE_ERRNO(temp);
    }

    return ret == 0;
}

int os_map_file_to_memory(struct os_mapped_file_data *mapped,
                          const char *filename)
{
    msg_log_assert(mapped != NULL);
    msg_log_assert(filename != NULL);

    errno = 0;

    while((mapped->fd = open(filename, O_RDONLY)) == -1 && errno == EINTR)
        ;

    if(mapped->fd < 0)
    {
        if(!verbosity.suppress_errors)
        {
            SAVE_ERRNO(temp);
            msg_error(errno, LOG_ERR, "Failed to open() file \"%s\"", filename);
            RESTORE_ERRNO(temp);
        }

        return -1;
    }

    struct stat buf;
    if(fstat(mapped->fd, &buf) < 0)
    {
        if(!verbosity.suppress_errors)
        {
            SAVE_ERRNO(temp);
            msg_error(errno, LOG_ERR, "Failed to fstat() file \"%s\"", filename);
            RESTORE_ERRNO(temp);
        }

        goto error_exit;
    }

    mapped->length = buf.st_size;

    if(mapped->length == 0 && S_ISBLK(buf.st_mode))
    {
        uint64_t device_size;
        if(ioctl(mapped->fd, BLKGETSIZE64, &device_size) == 0 &&
           device_size <= SIZE_MAX)
        {
            mapped->length = (size_t)device_size;
        }
    }

    if(mapped->length == 0)
    {
        msg_error(EINVAL, LOG_ERR, "Refusing to map empty file \"%s\"", filename);
        errno = EINVAL;
        goto error_exit;
    }

    mapped->ptr =
        mmap(NULL, mapped->length, PROT_READ, MAP_PRIVATE, mapped->fd, 0);

    if(mapped->ptr == MAP_FAILED)
    {
        if(!verbosity.suppress_errors)
        {
            SAVE_ERRNO(temp);
            msg_error(errno, LOG_ERR, "Failed to mmap() file \"%s\"", filename);
            RESTORE_ERRNO(temp);
        }

        goto error_exit;
    }

    return 0;

error_exit:
    safe_close_fd(mapped->fd);
    mapped->fd = -1;

    return -1;
}

void os_unmap_file(struct os_mapped_file_data *mapped)
{
    msg_log_assert(mapped != NULL);

    if(mapped->fd < 0)
    {
        errno = EBADF;
        return;
    }

    errno = 0;

    (void)munmap(mapped->ptr, mapped->length);

    safe_close_fd(mapped->fd);
    mapped->fd = -1;
}

void os_sync_dir(const char *path)
{
    msg_log_assert(path != NULL);

    errno = 0;

    int fd;

    while((fd = open(path, O_DIRECTORY | O_RDONLY)) == -1 && errno == EINTR)
        ;

    if(fd < 0)
    {
        if(!verbosity.suppress_errors)
        {
            SAVE_ERRNO(temp);
            msg_error(errno, LOG_ERR,
                      "Failed to open directory \"%s\" for syncing", path);
            RESTORE_ERRNO(temp);
        }
    }
    else
        safe_close_fd(fd);
}

int os_clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    return clock_gettime(clk_id, tp);
}

void os_nanosleep(const struct timespec *tp)
{
    struct timespec remaining = *tp;

    while(nanosleep(&remaining, &remaining) == -1 && errno == EINTR)
        ;
}

void os_sched_yield(void)
{
    sched_yield();
}
