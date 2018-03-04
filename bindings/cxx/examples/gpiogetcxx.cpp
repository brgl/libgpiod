/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/* Simplified C++ reimplementation of the gpioget tool. */

#include <gpiod.hpp>

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
	if (argc < 3) {
		::std::cerr << "usage: " << argv[0] << " <chip> <line_offset0> ..." << ::std::endl;
		return EXIT_FAILURE;
	}

	::std::vector<unsigned int> offsets;

	for (int i = 2; i < argc; i++)
		offsets.push_back(::std::stoul(argv[i]));

	::gpiod::chip chip(argv[1]);
	auto lines = chip.get_lines(offsets);

	lines.request({
		argv[0],
		::gpiod::line_request::DIRECTION_INPUT,
		0
	});

	auto vals = lines.get_values();

	for (auto& it: vals)
		::std::cout << it << ' ';
	::std::cout << ::std::endl;

	return EXIT_SUCCESS;
}
