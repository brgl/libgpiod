// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

/* Minimal example of reading multiple lines. */

#include <cstdlib>
#include <gpiod.hpp>
#include <iostream>

namespace {

/* Example configuration - customize to suit your situation */
const ::std::filesystem::path chip_path("/dev/gpiochip0");
const ::gpiod::line::offsets line_offsets = { 5, 3, 7 };

} /* namespace */

int main()
{
	auto request = ::gpiod::chip(chip_path)
			       .prepare_request()
			       .set_consumer("get-multiple-line-values")
			       .add_line_settings(
				       line_offsets,
				       ::gpiod::line_settings().set_direction(
					       ::gpiod::line::direction::INPUT))
			       .do_request();

	auto values = request.get_values();

	for (size_t i = 0; i < line_offsets.size(); i++)
		::std::cout << line_offsets[i] << "="
			    << (values[i] == ::gpiod::line::value::ACTIVE ?
					"Active" :
					"Inactive")
			    << ' ';
	::std::cout << ::std::endl;

	return EXIT_SUCCESS;
}
