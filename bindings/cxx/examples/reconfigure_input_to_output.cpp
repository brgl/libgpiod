// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

/*
 * Example of a bi-directional line requested as input and then switched
 * to output.
 */

#include <cstdlib>
#include <filesystem>
#include <gpiod.hpp>
#include <iostream>

namespace {

/* Example configuration - customize to suit your situation */
const ::std::filesystem::path chip_path("/dev/gpiochip0");
const ::gpiod::line::offset line_offset = 5;

} /* namespace */

int main()
{
	/* request the line initially as an input */
	auto request = ::gpiod::chip(chip_path)
			       .prepare_request()
			       .set_consumer("reconfigure-input-to-output")
			       .add_line_settings(
				       line_offset,
				       ::gpiod::line_settings().set_direction(
					       ::gpiod::line::direction::INPUT))
			       .do_request();

	/* read the current line value */
	::std::cout << line_offset << "="
		    << (request.get_value(line_offset) ==
					::gpiod::line::value::ACTIVE ?
				"Active" :
				"Inactive")
		    << " (input)" << ::std::endl;

	/* switch the line to an output and drive it low */
	request.reconfigure_lines(::gpiod::line_config().add_line_settings(
		line_offset,
		::gpiod::line_settings()
			.set_direction(::gpiod::line::direction::OUTPUT)
			.set_output_value(::gpiod::line::value::INACTIVE)));

	/* report the current driven value */
	::std::cout << line_offset << "="
		    << (request.get_value(line_offset) ==
					::gpiod::line::value::ACTIVE ?
				"Active" :
				"Inactive")
		    << " (output)" << ::std::endl;

	return EXIT_SUCCESS;
}
