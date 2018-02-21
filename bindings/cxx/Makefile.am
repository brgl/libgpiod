#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or (at
# your option) any later version.
#

lib_LTLIBRARIES = libgpiodcxx.la
libgpiodcxx_la_SOURCES = chip.cpp iter.cpp line.cpp line_bulk.cpp
libgpiodcxx_la_CPPFLAGS = -Wall -Wextra -g -std=gnu++11
libgpiodcxx_la_CPPFLAGS += -fvisibility=hidden -I$(top_srcdir)/include/
libgpiodcxx_la_LDFLAGS = -version-number $(subst .,:,$(PACKAGE_VERSION))
libgpiodcxx_la_LDFLAGS += -lgpiod -L$(top_builddir)/src/lib

include_HEADERS = gpiod.hpp

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpiodcxx.pc

SUBDIRS = . examples
