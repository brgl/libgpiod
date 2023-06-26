// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

/* Minimal example of reading the info for a line. */

#include <cstdlib>
#include <filesystem>
#include <gpiod.hpp>
#include <iomanip>
#include <iostream>

namespace {

/* Example configuration - customize to suit your situation */
const ::std::filesystem::path chip_path("/dev/gpiochip0");
const ::gpiod::line::offset line_offset = 3;

} /* namespace */

int main()
{
	auto chip = ::gpiod::chip(chip_path);
	auto info = chip.get_line_info(line_offset);

	::std::cout << "line " << ::std::setw(3) << info.offset() << ": "
		    << ::std::setw(12)
		    << (info.name().empty() ? "unnamed" : info.name()) << " "
		    << ::std::setw(12)
		    << (info.consumer().empty() ? "unused" : info.consumer())
		    << " " << ::std::setw(8)
		    << (info.direction() == ::gpiod::line::direction::INPUT ?
				"input" :
				"output")
		    << " " << ::std::setw(10)
		    << (info.active_low() ? "active-low" : "active-high")
		    << ::std::endl;

	return EXIT_SUCCESS;
}
