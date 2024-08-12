# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

noinst_LTLIBRARIES = libgpiosim-glib.la
libgpiosim_glib_la_SOURCES = \
	gpiosim-glib.c \
	gpiosim-glib.h

AM_CFLAGS = -I$(top_srcdir)/tests/gpiosim/
AM_CFLAGS += -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g -std=gnu89 $(GLIB_CFLAGS) $(GIO_CFLAGS)
AM_CFLAGS += -DG_LOG_DOMAIN=\"gpiosim-glib\"
libgpiosim_glib_la_LDFLAGS = -lgpiosim
