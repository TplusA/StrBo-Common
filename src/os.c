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

#include "os.h"
#include "messages.h"

int os_write_from_buffer(const void *src, size_t count, int fd)
{
    const uint8_t *src_ptr = src;

    while(count > 0)
    {
        ssize_t len;

        while((len = os_write(fd, src_ptr, count)) == -1 && errno == EINTR)
            ;

        if(len < 0)
        {
            msg_error(errno, LOG_ERR, "Failed writing to fd %d", fd);
            return -1;
        }

        log_assert((size_t)len <= count);

        src_ptr += len;
        count -= len;
    }

    return 0;
}

int os_try_read_to_buffer(void *dest, size_t count, size_t *dest_pos, int fd,
                          bool suppress_error_on_eagain)
{
    uint8_t *dest_ptr = dest;

    dest_ptr += *dest_pos;
    count -= *dest_pos;

    int retval = 0;

    while(count > 0)
    {
        const ssize_t len = os_read(fd, dest_ptr, count);

        if(len == 0)
            break;

        if(len < 0)
        {
            retval = (errno == EAGAIN) ? 0 : -1;

            if(errno != EAGAIN || !suppress_error_on_eagain)
                msg_error(errno, LOG_ERR, "Failed reading from fd %d", fd);

            break;
        }

        log_assert((size_t)len <= count);

        dest_ptr += len;
        count -= len;
        *dest_pos += len;
        retval = 1;
    }

    return retval;
}

void os_abort(void)
{
    abort();
}

int os_system(const char *command)
{
    msg_info("Executing external command: %s", command);

    const int ret = system(command);

    if(ret == EXIT_SUCCESS)
        msg_info("External command succeeded");
    else
        msg_error(0, LOG_ERR, "External command failed, exit code %d", ret);

    return ret;
}

int os_system_formatted(const char *format_string, ...)
{
    char buffer[1024];

    va_list va;
    va_start(va, format_string);

    (void)vsnprintf(buffer, sizeof(buffer), format_string, va);

    va_end(va);

    return os_system(buffer);
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

bool os_foreach_in_path(const char *path,
                        void (*callback)(const char *path, void *user_data),
                        void *user_data)
{
    log_assert(path != NULL);
    log_assert(callback != NULL);

    DIR *dir = opendir(path);

    if(dir == NULL)
    {
        msg_error(errno, LOG_ERR, "Failed opening directory \"%s\"", path);
        return false;
    }

    bool retval = true;

    while(true)
    {
        errno = 0;
        struct dirent *result = readdir(dir);

        if(result != NULL)
        {
            if(is_valid_directory_name(result->d_name))
                callback(result->d_name, user_data);
        }
        else
        {
            retval = (errno == 0);

            if(!retval)
                msg_error(errno, LOG_ERR,
                          "Failed reading directory \"%s\"", path);

            break;
        }
    }

    closedir(dir);

    return retval;
}

enum os_path_type os_path_get_type(const char *path)
{
    struct stat buf;

    if(stat(path, &buf) < 0)
    {
        msg_error(errno, LOG_ERR, "Failed to stat() file \"%s\"", path);
        return OS_PATH_TYPE_IO_ERROR;
    }

    if(S_ISDIR(buf.st_mode))
        return OS_PATH_TYPE_DIRECTORY;

    if(S_ISREG(buf.st_mode))
        return OS_PATH_TYPE_FILE;

    return OS_PATH_TYPE_OTHER;
}

char *os_resolve_symlink(const char *link)
{
    log_assert(link != NULL);

    char dummy;

    if(readlink(link, &dummy, sizeof(dummy)) < 0)
    {
        if(errno == EINVAL)
            msg_error(errno, LOG_NOTICE,
                      "Path \"%s\" is not a symlink", link);
        else
            msg_error(errno, LOG_NOTICE,
                      "readlink() failed for path \"%s\"", link);

        return NULL;
    }

    char *const result = realpath(link, NULL);

    if(result == NULL)
        msg_error(errno, LOG_NOTICE,
                  "Failed resolving symlink \"%s\"", link);

    return result;
}

bool os_mkdir_hierarchy(const char *path, bool must_not_exist)
{
    log_assert(path != NULL);

    if(must_not_exist)
    {
        struct stat buf;

        if(lstat(path, &buf) == 0)
        {
            msg_error(EEXIST, LOG_ERR, "Failed creating directory hierarchy %s", path);
            return false;
        }
    }

    /* oh well... */
    return os_system_formatted("mkdir -m 0750 -p %s", path) == EXIT_SUCCESS;
}

bool os_mkdir(const char *path, bool must_not_exist)
{
    log_assert(path != NULL);

    if(mkdir(path, 0750) == 0)
        return true;

    if(errno == EEXIST && !must_not_exist)
    {
        /* better make sure this is really a directory */
        struct stat buf;

        if(lstat(path, &buf) == 0 && S_ISDIR(buf.st_mode))
            return true;

        errno = EEXIST;
    }

    msg_error(errno, LOG_ERR, "Failed creating directory %s", path);

    return false;
}

bool os_rmdir(const char *path, bool must_exist)
{
    log_assert(path != NULL);

    if(rmdir(path) == 0)
        return true;

    if(must_exist)
        msg_error(errno, LOG_ERR, "Failed removing directory %s", path);

    return false;
}

int os_file_new(const char *filename)
{
    int fd;

    while((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC,
                     S_IRWXU | S_IRWXG | S_IRWXO)) == -1 &&
          errno == EINTR)
        ;

    if(fd < 0)
        msg_error(errno, LOG_ERR, "Failed to create file \"%s\"", filename);

    return fd;
}

static void safe_close_fd(int fd)
{
    if(fsync(fd) < 0 && errno != EINVAL)
        msg_error(errno, LOG_ERR, "fsync() fd %d", fd);

    int ret;
    while((ret = close(fd)) == -1 && errno == EINTR)
        ;

    if(ret == -1 && errno != EINTR)
        msg_error(errno, LOG_ERR, "Failed to close file descriptor %d", fd);
}

void os_file_close(int fd)
{
    if(fd < 0)
        msg_error(EINVAL, LOG_ERR,
                  "Passed invalid file descriptor %d to %s()", fd, __func__);
    else
        safe_close_fd(fd);
}

void os_file_delete(const char *filename)
{
    log_assert(filename != NULL);

    if(unlink(filename) < 0)
        msg_error(errno, LOG_ERR, "Failed to delete file \"%s\"", filename);
}

int os_map_file_to_memory(struct os_mapped_file_data *mapped,
                          const char *filename)
{
    log_assert(mapped != NULL);
    log_assert(filename != NULL);

    while((mapped->fd = open(filename, O_RDONLY)) == -1 && errno == EINTR)
        ;

    if(mapped->fd < 0)
    {
        msg_error(errno, LOG_ERR, "Failed to open() file \"%s\"", filename);
        return -1;
    }

    struct stat buf;
    if(fstat(mapped->fd, &buf) < 0)
    {
        msg_error(errno, LOG_ERR, "Failed to fstat() file \"%s\"", filename);
        goto error_exit;
    }

    mapped->length = buf.st_size;

    if(mapped->length == 0)
    {
        msg_error(errno, LOG_ERR, "Refusing to map empty file \"%s\"", filename);
        goto error_exit;
    }

    mapped->ptr =
        mmap(NULL, mapped->length, PROT_READ, MAP_PRIVATE, mapped->fd, 0);

    if(mapped->ptr == MAP_FAILED)
    {
        msg_error(errno, LOG_ERR, "Failed to mmap() file \"%s\"", filename);
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
    log_assert(mapped != NULL);

    if(mapped->fd < 0)
        return;

    (void)munmap(mapped->ptr, mapped->length);

    safe_close_fd(mapped->fd);
    mapped->fd = -1;
}

void os_sync_dir(const char *path)
{
    log_assert(path != NULL);

    int fd;

    while((fd = open(path, O_DIRECTORY | O_RDONLY)) == -1 && errno == EINTR)
        ;


    if(fd < 0)
        msg_error(errno, LOG_ERR,
                  "Failed to open directory \"%s\" for syncing", path);
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
