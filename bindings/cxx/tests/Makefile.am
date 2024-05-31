# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

AM_CXXFLAGS = -I$(top_srcdir)/bindings/cxx/ -I$(top_srcdir)/include
AM_CXXFLAGS += -I$(top_srcdir)/tests/gpiosim/
AM_CXXFLAGS += -Wall -Wextra -g -std=gnu++17 $(CATCH2_CFLAGS)
AM_LDFLAGS = -pthread $(CATCH2_LIBS)
LDADD = $(top_builddir)/bindings/cxx/libgpiodcxx.la
LDADD += $(top_builddir)/tests/gpiosim/libgpiosim.la

noinst_PROGRAMS = gpiod-cxx-test

gpiod_cxx_test_SOURCES = \
	check-kernel.cpp \
	gpiod-cxx-test-main.cpp \
	gpiosim.cpp \
	gpiosim.hpp \
	helpers.cpp \
	helpers.hpp \
	tests-chip.cpp \
	tests-chip-info.cpp \
	tests-edge-event.cpp \
	tests-info-event.cpp \
	tests-line.cpp \
	tests-line-config.cpp \
	tests-line-info.cpp \
	tests-line-request.cpp \
	tests-line-settings.cpp \
	tests-misc.cpp \
	tests-request-config.cpp
