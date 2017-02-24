#
# Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2.1 of the GNU Lesser General Public License
# as published by the Free Software Foundation.
#

AM_CFLAGS = -I$(top_srcdir)/include/ -include $(top_srcdir)/config.h
AM_CFLAGS += -Wall -Wextra -g
LDADD = -lgpiod -L$(top_srcdir)/src/lib -lkmod -ludev
DEPENDENCIES = libgpiod.la

check_PROGRAMS = gpiod-unit

gpiod_unit_SOURCES =	gpiod-unit.c \
			tests-chip.c \
			tests-iter.c \
			tests-line.c \
			tests-simple-api.c
