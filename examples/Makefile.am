# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

AM_CFLAGS = -I$(top_srcdir)/include/ -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g -std=gnu89

LDADD = $(top_builddir)/lib/libgpiod.la

noinst_PROGRAMS = \
	async_watch_line_value \
	find_line_by_name \
	get_chip_info \
	get_line_info \
	get_line_value \
	get_multiple_line_values \
	reconfigure_input_to_output \
	toggle_line_value \
	toggle_multiple_line_values \
	watch_line_info \
	watch_line_rising \
	watch_line_value \
	watch_multiple_line_values

async_watch_line_value_SOURCES = async_watch_line_value.c

find_line_by_name_SOURCES = find_line_by_name.c

get_chip_info_SOURCES = get_chip_info.c

get_line_info_SOURCES = get_line_info.c

get_line_value_SOURCES = get_line_value.c

get_multiple_line_values_SOURCES = get_multiple_line_values.c

reconfigure_input_to_output_SOURCES = reconfigure_input_to_output.c

toggle_line_value_SOURCES = toggle_line_value.c

toggle_multiple_line_value_SOURCES = toggle_multiple_line_value.c

watch_line_info_SOURCES = watch_line_info.c

watch_line_rising_SOURCES = watch_line_rising.c

watch_line_value_SOURCES = watch_line_value.c

watch_multiple_line_values_SOURCES = watch_multiple_line_values.c
