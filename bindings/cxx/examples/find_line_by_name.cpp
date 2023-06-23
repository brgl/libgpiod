// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

/* Minimal example of finding a line with the given name. */

#include <cstdlib>
#include <filesystem>
#include <gpiod.hpp>
#include <iostream>

namespace {

/* Example configuration - customize to suit your situation */
const ::std::string line_name = "GPIO19";

} /* namespace */

int main(void)
{
	/*
	 * Names are not guaranteed unique, so this finds the first line with
	 * the given name.
	 */
	for (const auto &entry :
	     ::std::filesystem::directory_iterator("/dev/")) {
		if (::gpiod::is_gpiochip_device(entry.path())) {
			::gpiod::chip chip(entry.path());

			auto offset = chip.get_line_offset_from_name(line_name);
			if (offset >= 0) {
				::std::cout << line_name << ": "
					    << chip.get_info().name() << " "
					    << offset << ::std::endl;
				return EXIT_SUCCESS;
			}
		}
	}
	::std::cout << "line '" << line_name << "' not found" << ::std::endl;

	return EXIT_FAILURE;
}
