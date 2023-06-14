# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

AM_CXXFLAGS = -I$(top_srcdir)/bindings/cxx/ -I$(top_srcdir)/include
AM_CXXFLAGS += -Wall -Wextra -g -std=gnu++17
AM_LDFLAGS = -lgpiodcxx -L$(top_builddir)/bindings/cxx/

noinst_PROGRAMS = \
	gpiodetectcxx \
	gpiofindcxx \
	gpiogetcxx \
	gpioinfocxx \
	gpiomoncxx \
	gpionotifycxx \
	gpiosetcxx \
	async_watch_line_value \
	get_line_value \
	toggle_line_value \
	watch_line_value

gpiodetectcxx_SOURCES = gpiodetectcxx.cpp

gpiofindcxx_SOURCES = gpiofindcxx.cpp

gpiogetcxx_SOURCES = gpiogetcxx.cpp

gpioinfocxx_SOURCES = gpioinfocxx.cpp

gpiomoncxx_SOURCES = gpiomoncxx.cpp

gpionotifycxx_SOURCES = gpionotifycxx.cpp

gpiosetcxx_SOURCES = gpiosetcxx.cpp

async_watch_line_value_SOURCES = async_watch_line_value.cpp

get_line_value_SOURCES = get_line_value.cpp

toggle_line_value_SOURCES = toggle_line_value.cpp

watch_line_value_SOURCES = watch_line_value.cpp
