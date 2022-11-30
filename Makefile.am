# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign
SUBDIRS = include lib

EXTRA_DIST = \
	LICENSES/GPL-2.0-or-later.txt \
	LICENSES/Apache-2.0.txt \
	LICENSES/LGPL-2.1-or-later.txt \
	LICENSES/CC-BY-SA-4.0.txt \
	LICENSES/CC0-1.0.txt \
	LICENSES/GPL-2.0-only.txt \
	LICENSES/Linux-syscall-note.txt \
	LICENSES/LGPL-3.0-or-later.txt \
	LICENSES/BSD-3-Clause.txt

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

EXTRA_DIST += Doxyfile

endif
