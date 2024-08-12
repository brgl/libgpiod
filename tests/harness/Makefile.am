# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

noinst_LTLIBRARIES = libgpiod-test-harness.la
libgpiod_test_harness_la_SOURCES = \
	gpiod-test.c \
	gpiod-test.h \
	gpiod-test-common.h

AM_CFLAGS = -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g -std=gnu89 $(GLIB_CFLAGS)
AM_CFLAGS += -DG_LOG_DOMAIN=\"gpiod-test\"
