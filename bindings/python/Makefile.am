# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

pyexec_LTLIBRARIES = gpiod.la

gpiod_la_SOURCES = gpiodmodule.c

gpiod_la_CFLAGS = -I$(top_srcdir)/include/
gpiod_la_CFLAGS += -Wall -Wextra -g -std=gnu89 $(PYTHON_CPPFLAGS)
gpiod_la_LDFLAGS = -module -avoid-version
gpiod_la_LIBADD = $(top_builddir)/lib/libgpiod.la $(PYTHON_LIBS)

SUBDIRS = .

if WITH_TESTS

SUBDIRS += tests

endif

if WITH_EXAMPLES

SUBDIRS += examples

endif
