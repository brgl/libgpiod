#
# Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#

AM_CFLAGS = -I$(top_srcdir)/include/ -include $(top_srcdir)/config.h
AM_CFLAGS += -Wall -Wextra -g -D_GNU_SOURCE -L$(top_srcdir)/src/lib
AM_LDFLAGS = -lgpiod
DEPENDENCIES = libgpiod.la

bin_PROGRAMS = gpiodetect gpioinfo gpioget gpioset gpiomon gpiofind

gpiodetect_SOURCES = gpiodetect.c tools-common.c

gpioinfo_SOURCES = gpioinfo.c tools-common.c

gpioget_SOURCES = gpioget.c tools-common.c

gpioset_SOURCES = gpioset.c tools-common.c

gpiomon_SOURCES = gpiomon.c tools-common.c

gpiofind_SOURCES = gpiofind.c tools-common.c
