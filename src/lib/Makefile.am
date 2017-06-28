#
# Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2.1 of the GNU Lesser General Public License
# as published by the Free Software Foundation.
#

lib_LTLIBRARIES = libgpiod.la
libgpiod_la_SOURCES = core.c iter.c misc.c simple.c
libgpiod_la_CFLAGS = -Wall -Wextra -g
libgpiod_la_CFLAGS += -fvisibility=hidden -I$(top_srcdir)/include/
libgpiod_la_CFLAGS += -include $(top_builddir)/config.h
libgpiod_la_LDFLAGS = -version-number $(subst .,:,$(PACKAGE_VERSION))
