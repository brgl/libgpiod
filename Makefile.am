#
# Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#

AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
LIBS =

AM_CPPFLAGS = -I$(top_srcdir) -include $(top_srcdir)/config.h -Wextra
AM_CPPFLAGS += -fvisibility=hidden -D_GNU_SOURCE

lib_LTLIBRARIES = libgpiod.la

include_HEADERS = gpiod.h

libgpiod_la_SOURCES = core.c

bin_PROGRAMS = gpiodetect gpioinfo gpioget gpioset gpiomon

gpiodetect_SOURCES = gpiodetect.c
gpiodetect_LDFLAGS = -lgpiod
gpiodetect_DEPENDENCIES = libgpiod.la

gpioinfo_SOURCES = gpioinfo.c
gpioinfo_LDFLAGS = -lgpiod
gpioinfo_DEPENDENCIES = libgpiod.la

gpioget_SOURCES = gpioget.c
gpioget_LDFLAGS = -lgpiod
gpioget_DEPENDENCIES = libgpiod.la

gpioset_SOURCES = gpioset.c
gpioset_LDFLAGS = -lgpiod
gpioset_DEPENDENCIES = libgpiod.la

gpiomon_SOURCES = gpiomon.c
gpiomon_LDFLAGS = -lgpiod
gpiomon_DEPENDENCIES = libgpiod.la

doc:
	doxygen
.PHONY: doc
