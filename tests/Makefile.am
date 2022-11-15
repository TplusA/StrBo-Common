#
# Copyright (C) 2015--2017, 2019, 2020, 2022  T+A elektroakustik GmbH & Co. KG
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

if WITH_CUTTER
TESTS = run_test.sh

if WITH_VALGRIND
TESTS += valgrind.sh
endif

AM_TESTS_ENVIRONMENT = CUTTER="$(CUTTER)" XSLTPROC="$(XSLTPROC)" VALGRIND="$(VALGRIND)"

AM_LDFLAGS = -module -rpath $(libdir) -avoid-version -no-undefined

AM_CPPFLAGS = $(CPPCUTTER_CFLAGS)
AM_CPPFLAGS += -I$(top_srcdir)/src

AM_CFLAGS = $(CWARNINGS)

AM_CXXFLAGS = $(CXXWARNINGS)

LIBS += $(CPPCUTTER_LIBS)

check_LTLIBRARIES = \
    test_inifile.la \
    test_md5.la \
    test_gvariantwrapper.la \
    test_stream_id.la

test_inifile_la_SOURCES = \
    test_inifile.cc \
    mock_os.hh mock_os.cc \
    mock_messages.hh mock_messages.cc \
    mock_expectation.hh
test_inifile_la_LIBADD = $(top_builddir)/src/libinifile.la
test_inifile_la_CFLAGS = $(AM_CFLAGS)
test_inifile_la_CXXFLAGS = $(AM_CXXFLAGS)

test_md5_la_SOURCES = test_md5.cc
test_md5_la_LIBADD = $(top_builddir)/src/libmd5.la
test_md5_la_CFLAGS = $(AM_CFLAGS)
test_md5_la_CXXFLAGS = $(AM_CXXFLAGS)

test_gvariantwrapper_la_SOURCES = test_gvariantwrapper.cc
test_gvariantwrapper_la_LIBADD = $(top_builddir)/src/libgvariantwrapper.la
test_gvariantwrapper_la_CFLAGS = $(AM_CFLAGS)
test_gvariantwrapper_la_CXXFLAGS = $(AM_CXXFLAGS)

test_stream_id_la_SOURCES = \
    test_stream_id.cc \
    ../src/stream_id.h ../src/stream_id.hh \
    mock_expectation.hh
test_stream_id_la_CFLAGS = $(AM_CFLAGS)
test_stream_id_la_CXXFLAGS = $(AM_CXXFLAGS)

CLEANFILES = test_report.xml test_report_junit.xml valgrind.xml

EXTRA_DIST = cutter2junit.xslt
EXTRA_DIST += cutter-1_2_4.supp
EXTRA_DIST += mock_backtrace.hh mock_backtrace.cc mock_timebase.hh

check-local: check-TESTS
	@if $(GREP) -w cutter $(TEST_LOGS); then \
	    echo "Unit tests failed (check log)"; \
	    exit 1; \
	fi
endif
