dnl Copyright (C) 2015, 2017, 2019, 2022  T+A elektroakustik GmbH & Co. KG
dnl Copyright (C) 2023  T+A elektroakustik GmbH & Co. KG
dnl
dnl This file is part of the T+A Streaming Board software stack ("StrBoWare").
dnl
dnl This program is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU General Public License
dnl as published by the Free Software Foundation; either version 2
dnl of the License, or (at your option) any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
dnl MA  02110-1301, USA.

AC_PREREQ([2.69])
AC_INIT([StrBo Common Code (StrBoWare)], [1.0], [r.tiemann@ta-hifi.com])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign])
AM_SILENT_RULES([yes])
AC_CONFIG_SRCDIR([src/inifile.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_USE_SYSTEM_EXTENSIONS()

AC_ARG_VAR([CWARNINGS], [strict set of C compiler warnings])
AC_ARG_VAR([CXXWARNINGS], [strict set of C++ compiler warnings])

AC_ARG_ENABLE([valgrind],
              [AS_HELP_STRING([--disable-valgrind],
                              [disable an extra run of unit tests with Valgrind on ``make check''])],
              [],
              [enable_valgrind=yes])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_AR
LT_INIT
PKG_PROG_PKG_CONFIG([0.26])
AC_CHECK_PROGS([XSLTPROC], [xsltproc])
AC_CHECK_PROGS([VALGRIND], [valgrind])

m4_ifdef([AC_CHECK_CUTTER],
[
AC_CHECK_CUTTER
AC_CHECK_CPPCUTTER
AC_CHECK_GCUTTER
AC_CHECK_COVERAGE
],
[ac_cv_use_cutter=no])

# Checks for libraries.
PKG_CHECK_MODULES([GVARIANTWRAPPER_DEPENDENCIES], [glib-2.0])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h inttypes.h stdlib.h string.h syslog.h unistd.h sys/ioctl.h])

m4_ifdef([AC_CHECK_CUTTER],
[
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$pkg_cv_CUTTER_CFLAGS"
AC_CHECK_HEADER([cutter.h], [], [ac_cv_use_cutter=no])
CPPFLAGS="$save_CPPFLAGS"

AC_LANG_PUSH([C++])
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$pkg_cv_CPPCUTTER_CFLAGS"
AC_CHECK_HEADER([cppcutter.h], [], [ac_cv_use_cutter=no])
CPPFLAGS="$save_CPPFLAGS"
AC_LANG_POP([C++])
])

AC_LANG_PUSH([C++])
AC_CHECK_HEADER([doctest.h])
AC_LANG_POP([C++])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_CHECK_FUNCS([clock_gettime memset mkdir munmap realpath rmdir strdup strerror])

AM_CONDITIONAL([WITH_CUTTER], [test "x$ac_cv_use_cutter" = "xyes"])
AM_CONDITIONAL([WITH_DOCTEST], [test "x$ac_cv_header_doctest_h" = "xyes"])
AM_CONDITIONAL([WITH_VALGRIND], [test "x$enable_valgrind" = "xyes"])

AC_CONFIG_FILES([Makefile src/Makefile src/dbus/Makefile tests_new/Makefile tests/Makefile])
AC_CONFIG_FILES([tests/run_test.sh], [chmod +x tests/run_test.sh])
AC_CONFIG_FILES([tests/valgrind.sh], [chmod +x tests/valgrind.sh])
AC_OUTPUT
