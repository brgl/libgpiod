# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

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
		tests-chip.c		\
		tests-ctxless.c		\
		tests-event.c		\
		tests-iter.c		\
		tests-line.c		\
		tests-misc.c

all-local: gpiod-test
	@echo " ********************************************************"
	@echo " * Core tests have been built as tests/gpiod-test.      *"
	@echo " *                                                      *"
	@echo " * They require a recent linux kernel version and the   *"
	@echo " * gpio-mockup module (must not be built-in).           *"
	@echo " *                                                      *"
	@echo " * Run the test executable with superuser privileges or *"
	@echo " * make sure /dev/gpiochipX files are readable and      *"
	@echo " * writable by normal users.                            *"
	@echo " ********************************************************"
