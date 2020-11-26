// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

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

			auto lines = chip.find_line(argv[1], true);
			if (!lines.empty()) {
				::std::cout << lines.front().name() << " " <<
					       lines.front().offset() << ::std::endl;
				return EXIT_SUCCESS;
			}
		}
	}

	return EXIT_FAILURE;
}
