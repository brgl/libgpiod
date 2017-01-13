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

include_HEADERS = gpiod.h

lib_LTLIBRARIES = libgpiod.la
libgpiod_la_SOURCES = core.c
libgpiod_la_CFLAGS = -g
libgpiod_la_LDFLAGS = -version-number $(VERSION_MAJOR):$(VERSION_MINOR):$(VERSION_RELEASE)

bin_PROGRAMS = gpiodetect gpioinfo gpioget gpioset gpiomon gpiofind

gpiodetect_SOURCES = gpiodetect.c tools-common.c
gpiodetect_LDFLAGS = -lgpiod
gpiodetect_DEPENDENCIES = libgpiod.la

gpioinfo_SOURCES = gpioinfo.c tools-common.c
gpioinfo_LDFLAGS = -lgpiod
gpioinfo_DEPENDENCIES = libgpiod.la

gpioget_SOURCES = gpioget.c tools-common.c
gpioget_LDFLAGS = -lgpiod
gpioget_DEPENDENCIES = libgpiod.la

gpioset_SOURCES = gpioset.c tools-common.c
gpioset_LDFLAGS = -lgpiod
gpioset_DEPENDENCIES = libgpiod.la

gpiomon_SOURCES = gpiomon.c tools-common.c
gpiomon_LDFLAGS = -lgpiod
gpiomon_DEPENDENCIES = libgpiod.la

gpiofind_SOURCES = gpiofind.c tools-common.c
gpiofind_LDFLAGS = -lgpiod
gpiofind_DEPENDENCIES = libgpiod.la

doc:
	doxygen
.PHONY: doc
