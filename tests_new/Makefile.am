#
# Copyright (C) 2018  T+A elektroakustik GmbH & Co. KG
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

if WITH_DOCTEST
check_PROGRAMS = \
    test_inifile \
    test_md5 \
    test_gvariantwrapper \
    test_stream_id

TESTS = $(check_PROGRAMS)

AM_CPPFLAGS = -DDOCTEST_CONFIG_TREAT_CHAR_STAR_AS_STRING
AM_CPPFLAGS += -I$(top_srcdir)/src
AM_CFLAGS = $(CWARNINGS)
AM_CXXFLAGS = $(CXXWARNINGS)

noinst_LTLIBRARIES = libtestrunner.la

libtestrunner_la_SOURCES = testrunner.cc

test_inifile_SOURCES = \
    test_inifile.cc \
    mock_os.hh mock_os.cc \
    mock_messages.hh mock_messages.cc \
    mock_expectation.hh
test_inifile_LDADD = \
    libtestrunner.la \
    $(top_builddir)/src/libinifile.la
test_inifile_CFLAGS = $(AM_CFLAGS)
test_inifile_CXXFLAGS = $(AM_CXXFLAGS)

test_md5_SOURCES = test_md5.cc
test_md5_LDADD = \
    libtestrunner.la \
    $(top_builddir)/src/libmd5.la
test_md5_CFLAGS = $(AM_CFLAGS)
test_md5_CXXFLAGS = $(AM_CXXFLAGS)

test_gvariantwrapper_SOURCES = test_gvariantwrapper.cc
test_gvariantwrapper_LDADD = \
    libtestrunner.la \
    $(top_builddir)/src/libgvariantwrapper.la
test_gvariantwrapper_CFLAGS = $(AM_CFLAGS)
test_gvariantwrapper_CXXFLAGS = $(AM_CXXFLAGS)

test_stream_id_SOURCES = \
    test_stream_id.cc \
    ../src/stream_id.h ../src/stream_id.hh
test_stream_id_LDADD = \
    libtestrunner.la \
    $(top_builddir)/src/libgvariantwrapper.la
test_stream_id_CFLAGS = $(AM_CFLAGS)
test_stream_id_CXXFLAGS = $(AM_CXXFLAGS)

doctest: $(check_PROGRAMS)
	for p in $(check_PROGRAMS); do ./$$p $(DOCTEST_EXTRA_OPTIONS); done
endif

