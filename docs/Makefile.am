# SPDX-License-Identifier: GPL-2.0-or-later
# SPDX-FileCopyrightText: 2024-2025 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

DOCS_DEPS = \
	bindings.rst \
	building.rst \
	conf.py \
	contributing.rst \
	core_api.rst \
	core_chip_info.rst \
	core_chips.rst \
	core_edge_event.rst \
	core_line_config.rst \
	core_line_defs.rst \
	core_line_info.rst \
	core_line_request.rst \
	core_line_settings.rst \
	core_line_watch.rst \
	core_misc.rst \
	core_request_config.rst \
	cpp_api.rst \
	cpp_chip_info.rst \
	cpp_chip.rst \
	cpp_edge_event_buffer.rst \
	cpp_edge_event.rst \
	cpp_exceptions.rst \
	cpp_info_event.rst \
	cpp_line_config.rst \
	cpp_line_info.rst \
	cpp_line_request.rst \
	cpp_line.rst \
	cpp_line_settings.rst \
	cpp_misc.rst \
	cpp_request_config.rst \
	dbus.rst \
	dbus_api.rst \
	Doxyfile \
	index.rst \
	glib_api.rst \
	gpio_tools.rst \
	gpiocli_top.rst \
	python_api.rst \
	python_chip_info.rst \
	python_chip.rst \
	python_edge_event.rst \
	python_exceptions.rst \
	python_info_event.rst \
	python_line_info.rst \
	python_line_request.rst \
	python_line.rst \
	python_line_settings.rst \
	python_misc.rst \
	rust_api.rst \
	testing.rst

docs: $(DOCS_DEPS)
	pushd ..; sphinx-build ./docs/ ./docs/sphinx-output; popd

.PHONY: docs

clean-local:
	rm -rf sphinx-output
	rm -rf doxygen-output

EXTRA_DIST = $(DOCS_DEPS) requirements.txt
