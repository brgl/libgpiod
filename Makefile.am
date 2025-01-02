# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>
# SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign
SUBDIRS = include lib contrib

EXTRA_DIST = \
	LICENSES/GPL-2.0-or-later.txt \
	LICENSES/Apache-2.0.txt \
	LICENSES/LGPL-2.1-or-later.txt \
	LICENSES/CC-BY-SA-4.0.txt \
	LICENSES/CC0-1.0.txt \
	LICENSES/GPL-2.0-only.txt \
	LICENSES/Linux-syscall-note.txt \
	LICENSES/BSD-3-Clause.txt

if WITH_EXAMPLES

SUBDIRS += examples

endif

if WITH_TOOLS

SUBDIRS += tools man

endif

if WITH_TESTS

SUBDIRS += tests

endif

# Build bindings after core tests. When building tests for bindings, we need
# libgpiosim to be already present.
SUBDIRS += bindings

if WITH_DBUS

SUBDIRS += dbus

endif

if WITH_DOCS

DOCS_DEPS = \
	docs/conf.py \
	docs/bindings.rst \
	docs/core_api.rst \
	docs/core_chip_info.rst \
	docs/core_chips.rst \
	docs/core_edge_event.rst \
	docs/core_line_config.rst \
	docs/core_line_defs.rst \
	docs/core_line_info.rst \
	docs/core_line_request.rst \
	docs/core_line_settings.rst \
	docs/core_line_watch.rst \
	docs/core_misc.rst \
	docs/core_request_config.rst \
	docs/cpp_api.rst \
	docs/cpp_chip_info.rst \
	docs/cpp_chip.rst \
	docs/cpp_edge_event_buffer.rst \
	docs/cpp_edge_event.rst \
	docs/cpp_exceptions.rst \
	docs/cpp_info_event.rst \
	docs/cpp_line_config.rst \
	docs/cpp_line_info.rst \
	docs/cpp_line_request.rst \
	docs/cpp_line.rst \
	docs/cpp_line_settings.rst \
	docs/cpp_misc.rst \
	docs/cpp_request_config.rst \
	docs/Doxyfile \
	docs/index.rst \
	docs/requirements.txt

docs: $(DOCS_DEPS)
	sphinx-build docs/ docs/sphinx-output

.PHONY: docs

clean-local:
	rm -rf docs/sphinx-output
	rm -rf docs/doxygen-output

EXTRA_DIST += $(DOCS_DEPS)

endif
