# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

lib_LTLIBRARIES = libgpiosim.la
noinst_PROGRAMS = gpiosim-selftest

AM_CFLAGS = -Wall -Wextra -g -fvisibility=hidden -std=gnu89
AM_CFLAGS += -include $(top_builddir)/config.h

libgpiosim_la_SOURCES = gpiosim.c gpiosim.h
libgpiosim_la_CFLAGS = $(AM_CFLAGS) $(KMOD_CFLAGS) $(MOUNT_CFLAGS)
libgpiosim_la_LDFLAGS = -version-info $(subst .,:,$(ABI_GPIOSIM_VERSION))
libgpiosim_la_LDFLAGS += $(KMOD_LIBS) $(MOUNT_LIBS) -pthread

gpiosim_selftest_SOURCES = gpiosim-selftest.c
gpiosim_selftest_LDADD = libgpiosim.la
