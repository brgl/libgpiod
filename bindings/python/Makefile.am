# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

SUBDIRS = . examples

pyexec_LTLIBRARIES = gpiod.la

gpiod_la_SOURCES = gpiodmodule.c

gpiod_la_CFLAGS = -I$(top_srcdir)/include/
gpiod_la_CFLAGS += -Wall -Wextra -g -std=gnu89 $(PYTHON_CPPFLAGS)
gpiod_la_LDFLAGS = -module -avoid-version
gpiod_la_LIBADD = $(top_builddir)/lib/libgpiod.la $(PYTHON_LIBS)

if WITH_TESTS

SUBDIRS += tests

endif
