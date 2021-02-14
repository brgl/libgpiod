# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

AM_CPPFLAGS = -I$(top_srcdir)/bindings/cxx/ -I$(top_srcdir)/include
AM_CPPFLAGS += -I$(top_srcdir)/tests/mockup/
AM_CPPFLAGS += -Wall -Wextra -g -std=gnu++11 $(CATCH2_CFLAGS)
AM_LDFLAGS = -lgpiodcxx -L$(top_builddir)/bindings/cxx/
AM_LDFLAGS += -lgpiomockup -L$(top_builddir)/tests/mockup/
AM_LDFLAGS += -pthread

bin_PROGRAMS = gpiod-cxx-test

gpiod_cxx_test_SOURCES =			\
		gpiod-cxx-test-main.cpp		\
		gpiod-cxx-test.cpp		\
		gpio-mockup.cpp			\
		gpio-mockup.hpp			\
		tests-chip.cpp			\
		tests-event.cpp			\
		tests-iter.cpp			\
		tests-line.cpp
