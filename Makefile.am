#
# Copyright (C) 2015, 2018  T+A elektroakustik GmbH & Co. KG
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

SUBDIRS = . src tests_new tests

ACLOCAL_AMFLAGS = -I m4

if WITH_DOCTEST
doctest:
	$(MAKE) $(AM_MAKEFLAGS) -C tests_new $@

doctest-valgrind:
	$(MAKE) $(AM_MAKEFLAGS) -C tests_new $@
endif
