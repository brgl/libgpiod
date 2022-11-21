# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

AM_CFLAGS = -I$(top_srcdir)/include/ -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g -std=gnu89

noinst_LTLIBRARIES = libtools-common.la
libtools_common_la_SOURCES = tools-common.c tools-common.h

LDADD = libtools-common.la $(top_builddir)/lib/libgpiod.la

if WITH_GPIOSET_INTERACTIVE

AM_CFLAGS += -DGPIOSET_INTERACTIVE
LDADD += $(LIBEDIT_LIBS)

endif

bin_PROGRAMS = gpiodetect gpioinfo gpioget gpioset gpiomon gpionotify

gpiodetect_SOURCES = gpiodetect.c

gpioinfo_SOURCES = gpioinfo.c

gpioget_SOURCES = gpioget.c

gpioset_SOURCES = gpioset.c

gpiomon_SOURCES = gpiomon.c

gpionotify_SOURCES = gpionotify.c

EXTRA_DIST = gpio-tools-test gpio-tools-test.bats

if WITH_TESTS

bin_SCRIPTS = gpio-tools-test gpio-tools-test.bats

endif
