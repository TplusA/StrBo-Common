#
# Copyright (C) 2015--2019  T+A elektroakustik GmbH & Co. KG
#
# This file is part of the T+A Streaming Board software stack ("StrBoWare").
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301, USA.
#

ACLOCAL_AMFLAGS = -I ../m4

noinst_LTLIBRARIES = libinifile.la libmd5.la libgvariantwrapper.la

libinifile_la_SOURCES = inifile.c inifile.h messages.h os.h
libinifile_la_CFLAGS = $(AM_CFLAGS)

libmd5_la_SOURCES = md5.cc md5.hh
libmd5_la_CFLAGS = $(AM_CFLAGS)
libmd5_la_CXXFLAGS = $(AM_CXXFLAGS)

libgvariantwrapper_la_SOURCES = gvariantwrapper.cc gvariantwrapper.hh
libgvariantwrapper_la_CPPFLAGS = $(GVARIANTWRAPPER_DEPENDENCIES_CFLAGS)
libgvariantwrapper_la_CFLAGS = $(AM_CFLAGS)
libgvariantwrapper_la_CXXFLAGS = $(AM_CXXFLAGS)

EXTRA_DIST = \
    messages.c \
    messages_glib.c messages_glib.h \
    messages_dbus.c messages_dbus.h \
    configuration.cc configuration.hh configuration_base.hh \
    configuration_changed.hh configuration_settings.hh \
    gerrorwrapper.hh \
    maybe.hh guard.hh \
    os.c os.hh breakpoint.h \
    hexdump.c hexdump.h \
    actor_id.h
