# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2019 Bartosz Golaszewski <bartekgola@gmail.com>
#

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign
SUBDIRS = include lib bindings

if WITH_TOOLS

SUBDIRS += tools man

endif

if WITH_TESTS

SUBDIRS += tests

endif

if HAS_DOXYGEN

doc:
	@(cat Doxyfile; \
		echo PROJECT_NUMBER = $(VERSION_STR); \
		echo INPUT += bindings/cxx/gpiod.hpp) | doxygen -
.PHONY: doc

clean-local:
	rm -rf doc

EXTRA_DIST = Doxyfile

endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgpiod.pc
