#
# Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#

lib_LTLIBRARIES = libgpiod.la
libgpiod_la_SOURCES = core.c
libgpiod_la_CFLAGS = -Wall -Wextra -g
libgpiod_la_CFLAGS += -fvisibility=hidden -I$(top_srcdir)/include/
libgpiod_la_CFLAGS += -include $(top_srcdir)/config.h
libgpiod_la_LDFLAGS = -version-number $(subst .,:,$(PACKAGE_VERSION))
