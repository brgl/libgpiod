# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022-2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

noinst_PROGRAMS = \
	find_line_by_name_glib \
	get_chip_info_glib \
	get_line_info_glib \
	get_line_value_glib \
	get_multiple_line_values_glib \
	reconfigure_input_to_output_glib \
	toggle_line_value_glib \
	toggle_multiple_line_values_glib \
	watch_line_info_glib \
	watch_line_value_glib \
	watch_multiple_edge_rising_glib

AM_CFLAGS = -I$(top_srcdir)/bindings/glib/
AM_CFLAGS += -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g -std=gnu89 $(GLIB_CFLAGS) $(GOBJECT_CFLAGS)
AM_CFLAGS += -DG_LOG_DOMAIN=\"gpiotools-glib\"
LDADD = $(top_builddir)/bindings/glib/libgpiod-glib.la
LDADD += $(GLIB_LIBS) $(GOBJECT_LIBS)
