// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

/* Minimal example of watching for requests on particular lines. */

#include <cstdlib>
#include <gpiod.hpp>
#include <iomanip>
#include <iostream>

namespace {

/* Example configuration - customize to suit your situation. */
const ::std::filesystem::path chip_path("/dev/gpiochip0");
const ::gpiod::line::offsets line_offsets = { 5, 3, 7 };

const char *event_type(const ::gpiod::info_event &event)
{
	switch (event.type()) {
	case ::gpiod::info_event::event_type::LINE_REQUESTED:
		return "Requested";
	case ::gpiod::info_event::event_type::LINE_RELEASED:
		return "Released";
	case ::gpiod::info_event::event_type::LINE_CONFIG_CHANGED:
		return "Reconfig";
	default:
		return "Unknown";
	}
}

} /* namespace */

int main()
{
	::gpiod::chip chip(chip_path);

	for (auto offset :line_offsets)
		chip.watch_line_info(offset);

	for (;;) {
		/* Blocks until at least one event is available */
		auto event = chip.read_info_event();
		::std::cout << "line: " << event.get_line_info().offset() << " "
			    << ::std::setw(9) << ::std::left
			    << event_type(event) << " "
			    << event.timestamp_ns() / 1000000000 << "."
			    << event.timestamp_ns() % 1000000000 << ::std::endl;
	}
}
