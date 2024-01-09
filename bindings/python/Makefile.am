# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

EXTRA_DIST = \
	MANIFEST.in \
	setup.py

if WITH_TESTS

python-tests:
	TOP_SRCDIR=$(abs_top_builddir) \
	TOP_BUILDDIR=$(abs_top_builddir) \
	$(PYTHON) build_tests.py

python-tests-run:
	PYTHONPATH=$(abs_top_srcdir)/bindings/python
	LD_LIBRARY_PATH=$(abs_top_builddir)/lib/.libs/:\
		$(abs_top_builddir)/tests/gpiosim/.libs/ \
	python3 -B -m tests

else

python-tests:

endif

clean-local:
	rm -rf dist
	rm -f gpiod/_ext.*.so
	rm -f tests/*/_ext.*.so

all-local: python-tests
	CFLAGS="-I$(abs_top_srcdir)/include/ -I$(abs_top_srcdir)/tests/gpiosim \
	-L$(abs_top_builddir)/lib/.libs/ -L$(abs_top_builddir)/tests/gpiosim/.libs/" \
	$(PYTHON) -m build

install-exec-local:
	$(PYTHON) -m pip install dist/*.whl \
	--ignore-installed --prefix=$(prefix) --root=$(if $(DESTDIR),$(DESTDIR),/)

SUBDIRS = gpiod

if WITH_TESTS

SUBDIRS += tests

endif

if WITH_EXAMPLES

SUBDIRS += examples

endif
