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

/* Simplified C++ reimplementation of the gpiomon tool. */

#include <gpiod.hpp>

#include <cstdlib>
#include <iostream>

namespace {

void print_event(const ::gpiod::line_event& event)
{
	if (event.event_type == ::gpiod::line_event::RISING_EDGE)
		::std::cout << " RISING EDGE";
	else if (event.event_type == ::gpiod::line_event::FALLING_EDGE)
		::std::cout << "FALLING EDGE";
	else
		throw ::std::logic_error("invalid event type");

	::std::cout << " ";

	::std::cout << ::std::chrono::duration_cast<::std::chrono::seconds>(event.timestamp).count();
	::std::cout << ".";
	::std::cout << event.timestamp.count() % 1000000000;

	::std::cout << " line: " << event.source.offset();

	::std::cout << ::std::endl;
}

} /* namespace */

int main(int argc, char **argv)
{
	if (argc < 3) {
		::std::cout << "usage: " << argv[0] << " <chip> <offset0> ..." << ::std::endl;
		return EXIT_FAILURE;
	}

	::std::vector<unsigned int> offsets;
	offsets.reserve(argc);
	for (int i = 2; i < argc; i++)
		offsets.push_back(::std::stoul(argv[i]));

	::gpiod::chip chip(argv[1]);
	auto lines = chip.get_lines(offsets);

	lines.request({
		argv[0],
		::gpiod::line_request::EVENT_BOTH_EDGES,
		0,
	});

	for (;;) {
		auto events = lines.event_wait(::std::chrono::nanoseconds(1000000000));
		if (events) {
			for (auto& it: events)
				print_event(it.event_read());
		}
	}

	return EXIT_SUCCESS;
}
