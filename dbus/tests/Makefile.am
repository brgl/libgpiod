# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

noinst_PROGRAMS = gpiodbus-test
gpiodbus_test_SOURCES = \
	daemon-process.c \
	daemon-process.h \
	helpers.c \
	helpers.h \
	tests-chip.c \
	tests-line.c \
	tests-request.c

AM_CFLAGS = -I$(top_srcdir)/tests/gpiosim-glib/
AM_CFLAGS += -I$(top_builddir)/dbus/lib/ -I$(top_srcdir)/dbus/lib/
AM_CFLAGS += -I$(top_srcdir)/tests/harness/
AM_CFLAGS += -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g -std=gnu89
AM_CFLAGS += $(GLIB_CFLAGS) $(GIO_CFLAGS)
AM_CFLAGS += -DG_LOG_DOMAIN=\"gpiodbus-test\"
LDADD = $(top_builddir)/tests/gpiosim/libgpiosim.la
LDADD += $(top_builddir)/tests/gpiosim-glib/libgpiosim-glib.la
LDADD += $(top_builddir)/tests/harness/libgpiod-test-harness.la
LDADD += $(top_builddir)/dbus/lib/libgpiodbus.la
LDADD += $(GLIB_LIBS) $(GIO_LIBS)
