# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

noinst_PROGRAMS = gpiod-glib-test
gpiod_glib_test_SOURCES = \
	helpers.c \
	helpers.h \
	tests-chip.c \
	tests-chip-info.c \
	tests-edge-event.c \
	tests-info-event.c \
	tests-line-config.c \
	tests-line-info.c \
	tests-line-request.c \
	tests-line-settings.c \
	tests-misc.c \
	tests-request-config.c

AM_CFLAGS = -I$(top_srcdir)/bindings/glib/
AM_CFLAGS += -I$(top_srcdir)/tests/gpiosim-glib/
AM_CFLAGS += -I$(top_srcdir)/tests/harness/
AM_CFLAGS += -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g -std=gnu89 $(GLIB_CFLAGS) $(GIO_CFLAGS)
AM_CFLAGS += -DG_LOG_DOMAIN=\"gpiod-glib-test\"
LDADD = $(top_builddir)/bindings/glib/libgpiod-glib.la
LDADD += $(top_builddir)/tests/gpiosim/libgpiosim.la
LDADD += $(top_builddir)/tests/gpiosim-glib/libgpiosim-glib.la
LDADD += $(top_builddir)/tests/harness/libgpiod-test-harness.la
LDADD += $(GLIB_LIBS) $(GIO_LIBS)
