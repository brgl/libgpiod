// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

/* C++ reimplementation of the gpiodetect tool. */

#include <cstdlib>
#include <filesystem>
#include <gpiod.hpp>
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
			auto info = chip.get_info();

			::std::cout << info.name() << " [" <<
				       info.label() << "] (" <<
				       info.num_lines() << " lines)" << ::std::endl;
		}
	}

	return EXIT_SUCCESS;
}
