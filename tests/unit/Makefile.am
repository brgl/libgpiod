#
# Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2.1 of the GNU Lesser General Public License
# as published by the Free Software Foundation.
#

AM_CFLAGS = -I$(top_srcdir)/include/ -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g $(KMOD_CFLAGS) $(UDEV_CFLAGS)
LDADD = ../../src/lib/libgpiod.la $(KMOD_LIBS) $(UDEV_LIBS)

check_PROGRAMS = gpiod-unit
TESTS = $(check_PROGRAMS)

gpiod_unit_SOURCES =	gpiod-unit.c \
			gpiod-unit.h \
			tests-chip.c \
			tests-iter.c \
			tests-line.c \
			tests-misc.c \
			tests-simple-api.c
