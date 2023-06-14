// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023 Kent Gibson <warthog618@gmail.com>

/* Minimal example of watching for edges on a single line. */

#include <cstdlib>
#include <filesystem>
#include <gpiod.hpp>
#include <iostream>

namespace {

/* Example configuration - customize to suit your situation. */
const ::std::filesystem::path chip_path("/dev/gpiochip0");
const ::gpiod::line::offset line_offset = 5;

const char *edge_event_type_str(const ::gpiod::edge_event &event)
{
	switch (event.type()) {
	case ::gpiod::edge_event::event_type::RISING_EDGE:
		return "Rising ";
	case ::gpiod::edge_event::event_type::FALLING_EDGE:
		return "Falling";
	default:
		return "Unknown";
	}
}

} /* namespace */

int main(void)
{
	/*
	 * Assume a button connecting the pin to ground, so pull it up and
	 * provide some debounce.
	 */
	auto request =
		::gpiod::chip(chip_path)
			.prepare_request()
			.set_consumer("watch-line-value")
			.add_line_settings(
				line_offset,
				::gpiod::line_settings()
					.set_direction(
						::gpiod::line::direction::INPUT)
					.set_edge_detection(
						::gpiod::line::edge::BOTH)
					.set_bias(::gpiod::line::bias::PULL_UP)
					.set_debounce_period(
						std::chrono::milliseconds(10)))
			.do_request();

	/*
	 * A larger buffer is an optimisation for reading bursts of events from
	 * the kernel, but that is not necessary in this case, so 1 is fine.
	 */
	::gpiod::edge_event_buffer buffer(1);

	for (;;) {
		/* Blocks until at least one event is available. */
		request.read_edge_events(buffer);

		for (const auto &event : buffer)
			::std::cout << "offset: " << event.line_offset()
				    << ", type: " << edge_event_type_str(event)
				    << ", event #" << event.line_seqno()
				    << ::std::endl;
	}

	return EXIT_SUCCESS;
}
