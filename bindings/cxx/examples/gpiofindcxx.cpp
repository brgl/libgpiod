// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

/* C++ reimplementation of the gpiofind tool. */

#include <gpiod.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>

int main(int argc, char **argv)
{
	if (argc != 2) {
		::std::cerr << "usage: " << argv[0] << " <line name>" << ::std::endl;
		return EXIT_FAILURE;
	}

	for (const auto& entry: ::std::filesystem::directory_iterator("/dev/")) {
		if (::gpiod::is_gpiochip_device(entry.path())) {
			::gpiod::chip chip(entry.path());

			auto offset = chip.find_line(argv[1]);
			if (offset >= 0) {
				::std::cout << chip.name() << " " << offset << ::std::endl;
				return EXIT_SUCCESS;
			}
		}
	}

	return EXIT_FAILURE;
}
