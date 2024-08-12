# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

AM_CFLAGS = -I$(top_srcdir)/bindings/glib/ -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g
AM_CFLAGS += -I$(top_builddir)/dbus/lib/ -I$(top_srcdir)/dbus/lib/
AM_CFLAGS += $(GLIB_CFLAGS) $(GIO_CFLAGS) $(GIO_UNIX_CFLAGS) $(GUDEV_CFLAGS)
AM_CFLAGS += -DG_LOG_DOMAIN=\"gpio-manager\"
AM_CFLAGS += $(PROFILING_CFLAGS)
AM_LDFLAGS = $(GLIB_LIBS) $(GIO_LIBS) $(GIO_UNIX_LIBS) $(GUDEV_LIBS)
AM_LDFLAGS += $(PROFILING_LDFLAGS)
LDADD = $(top_builddir)/bindings/glib/libgpiod-glib.la
LDADD += $(top_builddir)/dbus/lib/libgpiodbus.la

bin_PROGRAMS = gpio-manager
gpio_manager_SOURCES = \
	daemon.c \
	daemon.h \
	helpers.c \
	helpers.h \
	gpio-manager.c
