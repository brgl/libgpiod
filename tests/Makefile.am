# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2022 Bartosz Golaszewski <brgl@bgdev.pl>

SUBDIRS = mockup gpiosim

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
		tests-chip.c		\
		tests-event.c		\
		tests-line.c		\
		tests-misc.c
