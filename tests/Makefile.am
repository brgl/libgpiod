# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2022 Bartosz Golaszewski <brgl@bgdev.pl>

SUBDIRS = gpiosim

AM_CFLAGS = -I$(top_srcdir)/include/ -I$(top_srcdir)/tests/gpiosim/
AM_CFLAGS += -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g -std=gnu89 $(GLIB_CFLAGS) $(GIO_CFLAGS)
AM_CFLAGS += -DG_LOG_DOMAIN=\"gpiod-test\"
AM_CFLAGS += $(PROFILING_CFLAGS)
LDADD = $(top_builddir)/lib/libgpiod.la
LDADD += $(top_builddir)/tests/gpiosim/libgpiosim.la
LDADD += $(GLIB_LIBS) $(GIO_LIBS)

bin_PROGRAMS = gpiod-test

gpiod_test_SOURCES = \
	gpiod-test.c \
	gpiod-test.h \
	gpiod-test-helpers.c \
	gpiod-test-helpers.h \
	gpiod-test-sim.c \
	gpiod-test-sim.h \
	tests-chip.c \
	tests-chip-info.c \
	tests-edge-event.c \
	tests-info-event.c \
	tests-line-config.c \
	tests-line-info.c \
	tests-line-request.c \
	tests-line-settings.c \
	tests-misc.c \
	tests-request-config.c
