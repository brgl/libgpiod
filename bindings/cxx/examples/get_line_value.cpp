// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

/* Minimal example of reading a single line. */

#include <cstdlib>
#include <filesystem>
#include <gpiod.hpp>
#include <iostream>

namespace {

/* example configuration - customize to suit your situation */
const ::std::filesystem::path chip_path("/dev/gpiochip0");
const ::gpiod::line::offset line_offset = 5;

} /* namespace */

int main(void)
{
	auto request =
		::gpiod::chip(chip_path)
			.prepare_request()
			.set_consumer("get-line-value")
			.add_line_settings(
				line_offset,
				::gpiod::line_settings().set_direction(
					::gpiod::line::direction::INPUT))
			.do_request();

	::std::cout << request.get_value(line_offset) << ::std::endl;

	return EXIT_SUCCESS;
}
