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
	gpiosetcxx

gpiodetectcxx_SOURCES = gpiodetectcxx.cpp

gpiofindcxx_SOURCES = gpiofindcxx.cpp

gpiogetcxx_SOURCES = gpiogetcxx.cpp

gpioinfocxx_SOURCES = gpioinfocxx.cpp

gpiomoncxx_SOURCES = gpiomoncxx.cpp

gpionotifycxx_SOURCES = gpionotifycxx.cpp

gpiosetcxx_SOURCES = gpiosetcxx.cpp
