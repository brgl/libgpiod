# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

SUBDIRS = mockup

AM_CFLAGS = -I$(top_srcdir)/include/ -I$(top_srcdir)/tests/mockup/
AM_CFLAGS += -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g -std=gnu89 $(GLIB_CFLAGS)
AM_CFLAGS += -DG_LOG_DOMAIN=\"gpiod-test\"
AM_LDFLAGS = -pthread
LDADD = $(top_builddir)/lib/libgpiod.la
LDADD += $(top_builddir)/tests/mockup/libgpiomockup.la
LDADD += $(GLIB_LIBS)

bin_PROGRAMS = gpiod-test

gpiod_test_SOURCES =			\
		gpiod-test.c		\
		gpiod-test.h		\
		tests-bulk.c		\
		tests-chip.c		\
		tests-event.c		\
		tests-line.c		\
		tests-misc.c
