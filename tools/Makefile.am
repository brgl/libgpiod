# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

AM_CFLAGS = -I$(top_srcdir)/include/ -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g -std=gnu89

noinst_LTLIBRARIES = libtools-common.la
libtools_common_la_SOURCES = tools-common.c tools-common.h

include_HEADERS = gpiotools.h
lib_LTLIBRARIES = libgpiotools.la
libgpiotools_la_SOURCES = gpiotools.c
libgpiotools_la_CFLAGS = $(AM_CFLAGS)
libgpiotools_la_CFLAGS += -fvisibility=hidden -I$(top_srcdir)/include/
libgpiotools_la_CFLAGS += $(PROFILING_CFLAGS)
libgpiotools_la_LDFLAGS = -version-info $(subst .,:,$(ABI_GPIOTOOLS_VERSION))
libgpiotools_la_LDFLAGS += $(PROFILING_LDFLAGS)
libgpiotools_la_LDFLAGS += -lgpiod -L$(top_builddir)/lib

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpiotools.pc

LDADD = \
	libtools-common.la \
	libgpiotools.la \
	$(top_builddir)/lib/libgpiod.la

if WITH_GPIOSET_INTERACTIVE

AM_CFLAGS += -DGPIOSET_INTERACTIVE $(LIBEDIT_CFLAGS)
LDADD += $(LIBEDIT_LIBS)

endif

bin_PROGRAMS = gpiodetect gpioinfo gpioget gpioset gpiomon gpionotify

if WITH_TESTS

dist_noinst_SCRIPTS = gpio-tools-test.bash

endif
