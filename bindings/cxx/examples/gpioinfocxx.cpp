// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

/* Simplified C++ reimplementation of the gpioinfo tool. */

#include <gpiod.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {

void show_chip(const ::gpiod::chip& chip)
{
	auto info = chip.get_info();

	::std::cout << info.name() << " - " << info.num_lines() << " lines:" << ::std::endl;

	for (unsigned int offset = 0; offset < info.num_lines(); offset++) {
		auto info = chip.get_line_info(offset);

		::std::cout << "\tline ";
		::std::cout.width(3);
		::std::cout << info.offset() << ": ";

		::std::cout.width(12);
		::std::cout << (info.name().empty() ? "unnamed" : info.name());
		::std::cout << " ";

		::std::cout.width(12);
		::std::cout << (info.consumer().empty() ? "unused" : info.consumer());
		::std::cout << " ";

		::std::cout.width(8);
		::std::cout << (info.direction() == ::gpiod::line::direction::INPUT ? "input" : "output");
		::std::cout << " ";

		::std::cout.width(10);
		::std::cout << (info.active_low() ? "active-low" : "active-high");

		::std::cout << ::std::endl;
	}
}

} /* namespace */

int main(int argc, char **argv)
{
	if (argc != 1) {
		::std::cerr << "usage: " << argv[0] << ::std::endl;
		return EXIT_FAILURE;
	}

	for (const auto& entry: ::std::filesystem::directory_iterator("/dev/")) {
		if (::gpiod::is_gpiochip_device(entry.path()))
			show_chip(::gpiod::chip(entry.path()));
	}

	return EXIT_SUCCESS;
}
