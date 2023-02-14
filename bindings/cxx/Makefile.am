# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

lib_LTLIBRARIES = libgpiodcxx.la
libgpiodcxx_la_SOURCES = \
	chip.cpp \
	chip-info.cpp \
	edge-event-buffer.cpp \
	edge-event.cpp \
	exception.cpp \
	info-event.cpp \
	internal.cpp \
	internal.hpp \
	line.cpp \
	line-config.cpp \
	line-info.cpp \
	line-request.cpp \
	line-settings.cpp \
	misc.cpp \
	request-builder.cpp \
	request-config.cpp

libgpiodcxx_la_CXXFLAGS = -Wall -Wextra -g -std=gnu++17
libgpiodcxx_la_CXXFLAGS += -fvisibility=hidden -I$(top_srcdir)/include/
libgpiodcxx_la_CXXFLAGS += $(PROFILING_CFLAGS)
libgpiodcxx_la_LDFLAGS = -version-info $(subst .,:,$(ABI_CXX_VERSION))
libgpiodcxx_la_LDFLAGS += -lgpiod -L$(top_builddir)/lib
libgpiodcxx_la_LDFLAGS += $(PROFILING_LDFLAGS)

include_HEADERS = gpiod.hpp

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpiodcxx.pc

SUBDIRS = gpiodcxx .

if WITH_TESTS

SUBDIRS += tests

endif

if WITH_EXAMPLES

SUBDIRS += examples

endif
