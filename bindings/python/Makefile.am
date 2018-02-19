# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

SUBDIRS = . examples

pyexec_LTLIBRARIES = gpiod.la

gpiod_la_SOURCES = gpiodmodule.c

gpiod_la_CFLAGS = -I$(top_srcdir)/include/ -include $(top_builddir)/config.h
gpiod_la_CFLAGS += $(PYTHON_CPPFLAGS) -Wall -Wextra -g
gpiod_la_LDFLAGS = -module -avoid-version
gpiod_la_LIBADD = $(top_builddir)/src/lib/libgpiod.la
