/*
 * Copyright (C) 2018--2022  T+A elektroakustik GmbH & Co. KG
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

#include "mock_os.hh"

MockOS::Mock *MockOS::singleton = nullptr;


bool os_suppress_error_messages(bool do_suppress)
{
    REQUIRE(MockOS::singleton != nullptr);
    return MockOS::singleton->suppress_error_messages(do_suppress);
}

int os_write_from_buffer(const void *src, size_t count, int fd)
{
    REQUIRE(MockOS::singleton != nullptr);
    return MockOS::singleton->check_next<MockOS::WriteFromBuffer>(src, count, fd);
}

void os_abort(void)
{
    REQUIRE(MockOS::singleton != nullptr);
    MockOS::singleton->check_next<MockOS::Abort>();
}

char *os_resolve_symlink(const char *link)
{
    REQUIRE(MockOS::singleton != nullptr);
    return MockOS::singleton->check_next<MockOS::ResolveSymlink>(link);
}

enum os_path_type os_path_get_type(const char *path)
{
    REQUIRE(MockOS::singleton != nullptr);
    return MockOS::singleton->check_next<MockOS::PathGetType>(path);
}

int os_file_new(const char *filename)
{
    REQUIRE(MockOS::singleton != nullptr);
    return MockOS::singleton->check_next<MockOS::FileNew>(filename);
}

void os_file_close(int fd)
{
    REQUIRE(MockOS::singleton != nullptr);
    MockOS::singleton->check_next<MockOS::FileClose>(fd);
}

int os_file_delete(const char *filename)
{
    REQUIRE(MockOS::singleton != nullptr);
    return MockOS::singleton->check_next<MockOS::FileDelete>(filename);
}

int os_map_file_to_memory(struct os_mapped_file_data *mapped,
                          const char *filename)
{
    REQUIRE(MockOS::singleton != nullptr);
    return MockOS::singleton->check_next<MockOS::MapFileToMemory>(mapped, filename);
}

void os_unmap_file(struct os_mapped_file_data *mapped)
{
    REQUIRE(MockOS::singleton != nullptr);
    MockOS::singleton->check_next<MockOS::UnmapFile>(mapped);
}

int os_foreach_in_path(const char *path,
                       int (*callback)(const char *path, unsigned char dtype,
                                       void *user_data),
                       void *user_data)
{
    REQUIRE(MockOS::singleton != nullptr);
    FAIL("Not implemented yet");
    return -1;
}

void os_sync_dir(const char *path)
{
    REQUIRE(MockOS::singleton != nullptr);
    FAIL("Not implemented yet");
}

int os_system_formatted(bool is_verbose, const char *format_string, ...)
{
    REQUIRE(MockOS::singleton != nullptr);
    FAIL("Not implemented yet");
    return -1;
}
