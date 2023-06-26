// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

/* Minimal example of reading the info for a chip. */

#include <cstdlib>
#include <filesystem>
#include <gpiod.hpp>
#include <iostream>

namespace {

/* Example configuration - customize to suit your situation */
const ::std::filesystem::path chip_path("/dev/gpiochip0");

} /* namespace */

int main()
{
	::gpiod::chip chip(chip_path);
	auto info = chip.get_info();

	::std::cout << info.name() << " [" << info.label() << "] ("
		    << info.num_lines() << " lines)" << ::std::endl;

	return EXIT_SUCCESS;
}
