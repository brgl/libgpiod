# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
#

lib_LTLIBRARIES = libgpiomockup.la

libgpiomockup_la_SOURCES = gpio-mockup.c gpio-mockup.h
libgpiomockup_la_CFLAGS = -Wall -Wextra -g -fvisibility=hidden
libgpiomockup_la_CFLAGS += -include $(top_builddir)/config.h
libgpiomockup_la_CFLAGS += $(KMOD_CFLAGS) $(UDEV_CFLAGS)
libgpiomockup_la_LDFLAGS = -version-info $(subst .,:,$(ABI_MOCKUP_VERSION))
libgpiomockup_la_LDFLAGS += $(KMOD_LIBS) $(UDEV_LIBS)
