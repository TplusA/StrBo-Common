#
# Copyright (C) 2015, 2016, 2017  T+A elektroakustik GmbH & Co. KG
#
# This file is part of the T+A Streaming Board software stack ("StrBoWare").
#
# StrBoWare is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 as
# published by the Free Software Foundation.
#
# StrBoWare is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with StrBoWare.  If not, see <http://www.gnu.org/licenses/>.
#

ACLOCAL_AMFLAGS = -I ../m4

noinst_LTLIBRARIES = libinifile.la libmd5.la

libinifile_la_SOURCES = inifile.c inifile.h messages.h os.h
libinifile_la_CFLAGS = $(AM_CFLAGS)

libmd5_la_SOURCES = md5.cc md5.hh
libmd5_la_CFLAGS = $(AM_CFLAGS)
libmd5_la_CXXFLAGS = $(AM_CXXFLAGS)

EXTRA_DIST = \
    messages.c \
    messages_glib.c messages_glib.h \
    messages_dbus.c messages_dbus.h \
    gvariantwrapper.cc gvariantwrapper.hh \
    os.c \
    hexdump.c hexdump.h \
    actor_id.h
