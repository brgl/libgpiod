# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

AM_CPPFLAGS = -I$(top_srcdir)/bindings/cxx/ -I$(top_srcdir)/include
AM_CPPFLAGS += -Wall -Wextra -g -std=gnu++17
AM_LDFLAGS = -lgpiodcxx -L$(top_builddir)/bindings/cxx/ -lstdc++fs

noinst_PROGRAMS =				\
		gpiodetectcxx			\
		gpiofindcxx			\
		gpiogetcxx			\
		gpioinfocxx			\
		gpiomoncxx			\
		gpiosetcxx

gpiodetectcxx_SOURCES = gpiodetectcxx.cpp

gpiofindcxx_SOURCES = gpiofindcxx.cpp

gpiogetcxx_SOURCES = gpiogetcxx.cpp

gpioinfocxx_SOURCES = gpioinfocxx.cpp

gpiomoncxx_SOURCES = gpiomoncxx.cpp

gpiosetcxx_SOURCES = gpiosetcxx.cpp
