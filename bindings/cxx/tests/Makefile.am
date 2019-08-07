# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

AM_CPPFLAGS = -I$(top_srcdir)/bindings/cxx/ -I$(top_srcdir)/include
AM_CPPFLAGS += -I$(top_srcdir)/tests/mockup/
AM_CPPFLAGS += -Wall -Wextra -g -std=gnu++11 $(CATCH2_CFLAGS)
AM_LDFLAGS = -lgpiodcxx -L$(top_builddir)/bindings/cxx/
AM_LDFLAGS += -lgpiomockup -L$(top_builddir)/tests/mockup/
AM_LDFLAGS += -pthread

bin_PROGRAMS = gpiod-cxx-test

gpiod_cxx_test_SOURCES =	gpiod-cxx-test.cpp \
				gpio-mockup.cpp \
				gpio-mockup.hpp \
				tests-chip.cpp \
				tests-event.cpp \
				tests-iter.cpp \
				tests-line.cpp
