# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2017-2019 Bartosz Golaszewski <bartekgola@gmail.com>
#

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign
SUBDIRS = include lib

if WITH_TOOLS

SUBDIRS += tools man

endif

if WITH_TESTS

SUBDIRS += tests

endif

# Build bindings after core tests. When building tests for bindings, we need
# libgpiomockup to be already present.
SUBDIRS += bindings

if HAS_DOXYGEN

doc: Doxyfile
	@doxygen Doxyfile
.PHONY: doc

clean-local:
	rm -rf doc

EXTRA_DIST = Doxyfile

endif
