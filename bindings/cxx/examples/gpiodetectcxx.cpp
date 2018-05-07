// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/* C++ reimplementation of the gpiodetect tool. */

#include <gpiod.hpp>

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
	if (argc != 1) {
		::std::cerr << "usage: " << argv[0] << ::std::endl;
		return EXIT_FAILURE;
	}

	for (auto& it: ::gpiod::make_chip_iter()) {
	        ::std::cout << it.name() << " ["
			  << it.label() << "] ("
			  << it.num_lines() << " lines)" << ::std::endl;
	}

	return EXIT_SUCCESS;
}
