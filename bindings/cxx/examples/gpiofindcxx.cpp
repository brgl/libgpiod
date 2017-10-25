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

	::gpiod::line line = ::gpiod::find_line(argv[1]);
	if (!line)
		return EXIT_FAILURE;

	::std::cout << line.get_chip().name() << " " << line.offset() << ::std::endl;

	return EXIT_SUCCESS;
}
