#
# Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2.1 of the GNU Lesser General Public License
# as published by the Free Software Foundation.
#

AM_CFLAGS = -I$(top_srcdir)/include/ -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g

noinst_LTLIBRARIES = libtools-common.la
libtools_common_la_SOURCES = \
	tools-common.c \
	tools-common.h

LDADD = libtools-common.la $(top_builddir)/src/lib/libgpiod.la

bin_PROGRAMS = gpiodetect gpioinfo gpioget gpioset gpiomon gpiofind

gpiodetect_SOURCES = gpiodetect.c

gpioinfo_SOURCES = gpioinfo.c

gpioget_SOURCES = gpioget.c

gpioset_SOURCES = gpioset.c

gpiomon_SOURCES = gpiomon.c

gpiofind_SOURCES = gpiofind.c
