# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

AM_CXXFLAGS = -I$(top_srcdir)/bindings/cxx/ -I$(top_srcdir)/include
AM_CXXFLAGS += -Wall -Wextra -g -std=gnu++17
AM_LDFLAGS = -lgpiodcxx -L$(top_builddir)/bindings/cxx/

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

async_watch_line_value_SOURCES = async_watch_line_value.cpp

find_line_by_name_SOURCES = find_line_by_name.cpp

get_chip_info_SOURCES = get_chip_info.cpp

get_line_info_SOURCES = get_line_info.cpp

get_line_value_SOURCES = get_line_value.cpp

get_multiple_line_values_SOURCES = get_multiple_line_values.cpp

reconfigure_input_to_output_SOURCES = reconfigure_input_to_output.cpp

toggle_line_value_SOURCES = toggle_line_value.cpp

toggle_multiple_line_values_SOURCES = toggle_multiple_line_values.cpp

watch_line_info_SOURCES = watch_line_info.cpp

watch_line_value_SOURCES = watch_line_value.cpp

watch_line_rising_SOURCES = watch_line_rising.cpp

watch_multiple_line_values_SOURCES = watch_multiple_line_values.cpp
