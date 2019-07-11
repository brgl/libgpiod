# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

SUBDIRS = mockup

AM_CFLAGS = -I$(top_srcdir)/include/ -I$(top_srcdir)/tests/mockup/
AM_CFLAGS += -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g
AM_LDFLAGS = -pthread
LDADD = $(top_builddir)/lib/libgpiod.la
LDADD += $(top_builddir)/tests/mockup/libgpiomockup.la

bin_PROGRAMS = gpiod-test

gpiod_test_SOURCES =	gpiod-test.c \
			gpiod-test.h \
			tests-chip.c \
			tests-ctxless.c \
			tests-event.c \
			tests-iter.c \
			tests-line.c \
			tests-misc.c

if WITH_TOOLS

gpiod_test_SOURCES +=	tests-gpiodetect.c \
			tests-gpiofind.c \
			tests-gpioget.c \
			tests-gpioinfo.c \
			tests-gpiomon.c \
			tests-gpioset.c

endif

all-local: gpiod-test
	@echo " ********************************************************"
	@echo " * Tests have been built as tests/gpiod-test.           *"
	@echo " *                                                      *"
	@echo " * They require a recent linux kernel version and the   *"
	@echo " * gpio-mockup module (must not be built-in).           *"
	@echo " *                                                      *"
	@echo " * Run the test executable with superuser privileges or *"
	@echo " * make sure /dev/gpiochipX files are readable and      *"
	@echo " * writable by normal users.                            *"
	@echo " ********************************************************"
