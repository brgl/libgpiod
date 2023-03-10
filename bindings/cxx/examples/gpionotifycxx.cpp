// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

/* Simplified C++ reimplementation of the gpionotify tool. */

#include <cstdlib>
#include <gpiod.hpp>
#include <iostream>

namespace {

void print_event(const ::gpiod::info_event& event)
{
	switch (event.type()) {
	case ::gpiod::info_event::event_type::LINE_REQUESTED:
		::std::cout << "LINE REQUESTED";
		break;
	case ::gpiod::info_event::event_type::LINE_RELEASED:
		::std::cout << "LINE RELEASED";
		break;
	case ::gpiod::info_event::event_type::LINE_CONFIG_CHANGED:
		::std::cout << "CONFIG CHANGED";
		break;
	}

	::std::cout << " ";

	::std::cout << event.timestamp_ns() / 1000000000;
	::std::cout << ".";
	::std::cout << event.timestamp_ns() % 1000000000;

	::std::cout << " line: " << event.get_line_info().offset();

	::std::cout << ::std::endl;
}

} /* namespace */

int main(int argc, char **argv)
{
	if (argc < 3) {
		::std::cout << "usage: " << argv[0] << " <chip> <offset0> ..." << ::std::endl;
		return EXIT_FAILURE;
	}

	::gpiod::chip chip(argv[1]);

	for (int i = 2; i < argc; i++)
		chip.watch_line_info(::std::stoul(argv[i]));

	for (;;)
		print_event(chip.read_info_event());

	return EXIT_SUCCESS;
}
