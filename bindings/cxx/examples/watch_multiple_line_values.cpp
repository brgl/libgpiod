// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

/* Minimal example of watching for edges on multiple lines. */

#include <cstdlib>
#include <gpiod.hpp>
#include <iomanip>
#include <iostream>

namespace {

/* Example configuration - customize to suit your situation */
const ::std::filesystem::path chip_path("/dev/gpiochip0");
const ::gpiod::line::offsets line_offsets = { 5, 3, 7 };

const char *edge_event_type_str(const ::gpiod::edge_event &event)
{
	switch (event.type()) {
	case ::gpiod::edge_event::event_type::RISING_EDGE:
		return "Rising";
	case ::gpiod::edge_event::event_type::FALLING_EDGE:
		return "Falling";
	default:
		return "Unknown";
	}
}

} /* namespace */

int main()
{
	auto request =
		::gpiod::chip(chip_path)
			.prepare_request()
			.set_consumer("watch-multiple-line-values")
			.add_line_settings(
				line_offsets,
				::gpiod::line_settings()
					.set_direction(
						::gpiod::line::direction::INPUT)
					.set_edge_detection(
						::gpiod::line::edge::BOTH))
			.do_request();

	::gpiod::edge_event_buffer buffer;

	for (;;) {
		/* Blocks until at least one event available */
		request.read_edge_events(buffer);

		for (const auto &event : buffer)
			::std::cout << "offset: " << event.line_offset()
				    << "  type: " << ::std::setw(7)
				    << ::std::left << edge_event_type_str(event)
				    << "  event #" << event.global_seqno()
				    << "  line event #" << event.line_seqno()
				    << ::std::endl;
	}
}
