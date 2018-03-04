# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

AM_CPPFLAGS = -I$(top_srcdir)/bindings/cxx/ -I$(top_srcdir)/include
AM_CPPFLAGS += -Wall -Wextra -g -std=gnu++11
AM_LDFLAGS = -lgpiodcxx -L$(top_builddir)/bindings/cxx/

check_PROGRAMS =	gpiod_cxx_tests \
			gpiodetectcxx \
			gpiofindcxx \
			gpiogetcxx \
			gpioinfocxx \
			gpiomoncxx \
			gpiosetcxx

gpiod_cxx_tests_SOURCES = gpiod_cxx_tests.cpp

gpiodetectcxx_SOURCES = gpiodetectcxx.cpp

gpiofindcxx_SOURCES = gpiofindcxx.cpp

gpiogetcxx_SOURCES = gpiogetcxx.cpp

gpioinfocxx_SOURCES = gpioinfocxx.cpp

gpiomoncxx_SOURCES = gpiomoncxx.cpp

gpiosetcxx_SOURCES = gpiosetcxx.cpp
