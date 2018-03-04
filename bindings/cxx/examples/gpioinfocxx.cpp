/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/* Simplified C++ reimplementation of the gpioinfo tool. */

#include <gpiod.hpp>

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
	if (argc != 1) {
		::std::cerr << "usage: " << argv[0] << ::std::endl;
		return EXIT_FAILURE;
	}

	for (auto& cit: ::gpiod::make_chip_iter()) {
		::std::cout << cit.name() << " - " << cit.num_lines() << " lines:" << ::std::endl;

		for (auto& lit: ::gpiod::line_iter(cit)) {
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
			::std::cout << (lit.active_state() == ::gpiod::line::ACTIVE_LOW
								? "active-low" : "active-high");

			::std::cout << ::std::endl;
		}
	}

	return EXIT_SUCCESS;
}
