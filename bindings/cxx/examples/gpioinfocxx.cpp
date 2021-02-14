// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

/* Simplified C++ reimplementation of the gpioinfo tool. */

#include <gpiod.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>

int main(int argc, char **argv)
{
	if (argc != 1) {
		::std::cerr << "usage: " << argv[0] << ::std::endl;
		return EXIT_FAILURE;
	}

	for (const auto& entry: ::std::filesystem::directory_iterator("/dev/")) {
		if (::gpiod::is_gpiochip_device(entry.path())) {
			::gpiod::chip chip(entry.path());

			::std::cout << chip.name() << " - " << chip.num_lines() << " lines:" << ::std::endl;

			for (auto& lit: ::gpiod::line_iter(chip)) {
				::std::cout << "\tline ";
				::std::cout.width(3);
				::std::cout << lit.offset() << ": ";

				::std::cout.width(12);
				::std::cout << (lit.name().empty() ? "unnamed" : lit.name());
				::std::cout << " ";

				::std::cout.width(12);
				::std::cout << (lit.consumer().empty() ? "unused" : lit.consumer());
				::std::cout << " ";

				::std::cout.width(8);
				::std::cout << (lit.direction() == ::gpiod::line::DIRECTION_INPUT ? "input" : "output");
				::std::cout << " ";

				::std::cout.width(10);
				::std::cout << (lit.is_active_low() ? "active-low" : "active-high");

				::std::cout << ::std::endl;
			}
		}
	}

	return EXIT_SUCCESS;
}
