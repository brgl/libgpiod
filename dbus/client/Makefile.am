# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

AM_CFLAGS = -include $(top_builddir)/config.h
AM_CFLAGS += -I$(top_builddir)/dbus/lib/ -I$(top_srcdir)/dbus/lib/
AM_CFLAGS += -Wall -Wextra -g
AM_CFLAGS += $(GLIB_CFLAGS) $(GIO_CFLAGS) $(GIO_UNIX_CFLAGS)
AM_CFLAGS += -DG_LOG_DOMAIN=\"gpiocli\"
AM_LDFLAGS = $(GLIB_LIBS) $(GIO_LIBS) $(GIO_UNIX_LIBS)
LDADD = $(top_builddir)/dbus/lib/libgpiodbus.la

bin_PROGRAMS = gpiocli

gpiocli_SOURCES = \
	common.c \
	common.h \
	detect.c \
	find.c \
	get.c \
	gpiocli.c \
	info.c \
	monitor.c \
	notify.c \
	reconfigure.c \
	release.c \
	request.c \
	requests.c \
	set.c \
	wait.c

dist_noinst_SCRIPTS = gpiocli-test.bash
