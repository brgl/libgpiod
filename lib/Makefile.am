# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

lib_LTLIBRARIES = libgpiod.la
libgpiod_la_SOURCES = core.c helpers.c iter.c misc.c
libgpiod_la_CFLAGS = -Wall -Wextra -g -std=gnu89
libgpiod_la_CFLAGS += -fvisibility=hidden -I$(top_srcdir)/include/
libgpiod_la_CFLAGS += -include $(top_builddir)/config.h
libgpiod_la_LDFLAGS = -version-info $(subst .,:,$(ABI_VERSION))

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpiod.pc
