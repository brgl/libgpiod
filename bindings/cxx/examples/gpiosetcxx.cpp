// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

/* Simplified C++ reimplementation of the gpioset tool. */

#include <gpiod.hpp>

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
	if (argc < 3) {
		::std::cerr << "usage: " << argv[0] <<
			       " <chip> <line_offset0>=<value0> ..." << ::std::endl;
		return EXIT_FAILURE;
	}

	::gpiod::line::offsets offsets;
	::gpiod::line::values values;

	for (int i = 2; i < argc; i++) {
		::std::string arg(argv[i]);

		size_t pos = arg.find('=');

		::std::string offset(arg.substr(0, pos));
		::std::string value(arg.substr(pos + 1, ::std::string::npos));

		if (offset.empty() || value.empty())
			throw ::std::invalid_argument("invalid offset=value mapping: " +
						      ::std::string(argv[i]));

		offsets.push_back(::std::stoul(offset));
		values.push_back(::std::stoul(value) ? ::gpiod::line::value::ACTIVE :
						       ::gpiod::line::value::INACTIVE);
	}

	auto request = ::gpiod::chip(argv[1])
		.prepare_request()
		.set_consumer("gpiosetcxx")
		.add_line_settings(
			offsets,
			::gpiod::line_settings()
				.set_direction(::gpiod::line::direction::OUTPUT)
		)
		.set_output_values(values)
		.do_request();

	::std::cin.get();

	return EXIT_SUCCESS;
}
