// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

/* Simplified C++ reimplementation of the gpiomon tool. */

#include <chrono>
#include <cstdlib>
#include <gpiod.hpp>
#include <iostream>

namespace {

void print_event(const ::gpiod::edge_event& event)
{
	if (event.type() == ::gpiod::edge_event::event_type::RISING_EDGE)
		::std::cout << " RISING EDGE";
	else
		::std::cout << "FALLING EDGE";

	::std::cout << " ";

	::std::cout << event.timestamp_ns() / 1000000000;
	::std::cout << ".";
	::std::cout << event.timestamp_ns() % 1000000000;

	::std::cout << " line: " << event.line_offset();

	::std::cout << ::std::endl;
}

} /* namespace */

int main(int argc, char **argv)
{
	if (argc < 3) {
		::std::cout << "usage: " << argv[0] << " <chip> <offset0> ..." << ::std::endl;
		return EXIT_FAILURE;
	}

	::gpiod::line::offsets offsets;
	offsets.reserve(argc);
	for (int i = 2; i < argc; i++)
		offsets.push_back(::std::stoul(argv[i]));

	auto request = ::gpiod::chip(argv[1])
		.prepare_request()
		.set_consumer("gpiomoncxx")
		.add_line_settings(
			offsets,
			::gpiod::line_settings()
				.set_direction(::gpiod::line::direction::INPUT)
				.set_edge_detection(::gpiod::line::edge::BOTH)
		)
		.do_request();

	::gpiod::edge_event_buffer buffer;

	for (;;) {
		request.read_edge_event(buffer);

		for (const auto& event: buffer)
			print_event(event);
	}

	return EXIT_SUCCESS;
}
