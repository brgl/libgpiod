# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

lib_LTLIBRARIES = libgpiod.la
libgpiod_la_SOURCES = core.c helpers.c misc.c uapi/gpio.h
libgpiod_la_CFLAGS = -Wall -Wextra -g -std=gnu89
libgpiod_la_CFLAGS += -fvisibility=hidden -I$(top_srcdir)/include/
libgpiod_la_CFLAGS += -include $(top_builddir)/config.h
libgpiod_la_LDFLAGS = -version-info $(subst .,:,$(ABI_VERSION))

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpiod.pc
