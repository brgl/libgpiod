// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/* C++ reimplementation of the gpiofind tool. */

#include <gpiod.hpp>

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
	if (argc != 2) {
		::std::cerr << "usage: " << argv[0] << " <line name>" << ::std::endl;
		return EXIT_FAILURE;
	}

	for (auto& chip: ::gpiod::make_chip_iter()) {
		auto line = chip.find_line(argv[1]);
		if (line) {
			::std::cout << line.name() << " " << line.offset() << ::std::endl;
			return EXIT_SUCCESS;
		}
	}

	return EXIT_FAILURE;
}
