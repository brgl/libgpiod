# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

lib_LTLIBRARIES = libgpiomockup.la

libgpiomockup_la_SOURCES = gpio-mockup.c gpio-mockup.h
libgpiomockup_la_CFLAGS = -Wall -Wextra -g -fvisibility=hidden -std=gnu89
libgpiomockup_la_CFLAGS += -include $(top_builddir)/config.h
libgpiomockup_la_CFLAGS += $(KMOD_CFLAGS) $(UDEV_CFLAGS)
libgpiomockup_la_LDFLAGS = -version-info $(subst .,:,$(ABI_MOCKUP_VERSION))
libgpiomockup_la_LDFLAGS += $(KMOD_LIBS) $(UDEV_LIBS)
