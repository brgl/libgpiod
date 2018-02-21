/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
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
