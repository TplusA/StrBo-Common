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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include "mock_os.hh"

MockOS::Mock *MockOS::singleton = nullptr;


bool os_suppress_error_messages(bool do_suppress)
{
    return MockOS::singleton->suppress_error_messages(do_suppress);
}

int os_write_from_buffer(const void *src, size_t count, int fd)
{
    return MockOS::singleton->check_next<MockOS::WriteFromBuffer>(src, count, fd);
}

void os_abort(void)
{
    MockOS::singleton->check_next<MockOS::Abort>();
}

int os_file_new(const char *filename)
{
    return MockOS::singleton->check_next<MockOS::FileNew>(filename);
}

void os_file_close(int fd)
{
    MockOS::singleton->check_next<MockOS::FileClose>(fd);
}

int os_file_delete(const char *filename)
{
    return MockOS::singleton->check_next<MockOS::FileDelete>(filename);
}

int os_map_file_to_memory(struct os_mapped_file_data *mapped,
                          const char *filename)
{
    return MockOS::singleton->check_next<MockOS::MapFileToMemory>(mapped, filename);
}

void os_unmap_file(struct os_mapped_file_data *mapped)
{
    MockOS::singleton->check_next<MockOS::UnmapFile>(mapped);
}