# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

lib_LTLIBRARIES = libgpiodcxx.la
libgpiodcxx_la_SOURCES = chip.cpp internal.h iter.cpp line.cpp line_bulk.cpp
libgpiodcxx_la_CPPFLAGS = -Wall -Wextra -g -std=gnu++11
libgpiodcxx_la_CPPFLAGS += -fvisibility=hidden -I$(top_srcdir)/include/
libgpiodcxx_la_LDFLAGS = -version-info $(subst .,:,$(ABI_CXX_VERSION))
libgpiodcxx_la_LDFLAGS += -lgpiod -L$(top_builddir)/lib

include_HEADERS = gpiod.hpp

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpiodcxx.pc

SUBDIRS = .

if WITH_TESTS

SUBDIRS += tests

endif

if WITH_EXAMPLES

SUBDIRS += examples

endif
