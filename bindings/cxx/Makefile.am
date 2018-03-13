# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

lib_LTLIBRARIES = libgpiodcxx.la
libgpiodcxx_la_SOURCES = chip.cpp iter.cpp line.cpp line_bulk.cpp
libgpiodcxx_la_CPPFLAGS = -Wall -Wextra -g -std=gnu++11
libgpiodcxx_la_CPPFLAGS += -fvisibility=hidden -I$(top_srcdir)/include/
libgpiodcxx_la_LDFLAGS = -version-info $(subst .,:,$(ABI_CXX_VERSION))
libgpiodcxx_la_LDFLAGS += -lgpiod -L$(top_builddir)/src/lib

include_HEADERS = gpiod.hpp

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpiodcxx.pc

SUBDIRS = . examples
