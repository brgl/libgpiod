// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

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

	::gpiod::line::offsets offsets;

	for (int i = 2; i < argc; i++)
		offsets.push_back(::std::stoul(argv[i]));

	auto request = ::gpiod::chip(argv[1])
		.prepare_request()
		.set_consumer("gpiogetcxx")
		.add_line_settings(
			offsets,
			::gpiod::line_settings()
				.set_direction(::gpiod::line::direction::INPUT)
		)
		.do_request();

	auto vals = request.get_values();

	for (auto& it: vals)
		::std::cout << (it == ::gpiod::line::value::ACTIVE ? "1" : "0") << ' ';
	::std::cout << ::std::endl;

	return EXIT_SUCCESS;
}
