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

	auto ret = ::gpiod::find_line(argv[1]);
	if (!ret.first)
		return EXIT_FAILURE;

	::std::cout << ret.second.name() << " " << ret.first.offset() << ::std::endl;

	return EXIT_SUCCESS;
}
