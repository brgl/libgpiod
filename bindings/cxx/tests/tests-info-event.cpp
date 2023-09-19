// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <catch2/catch.hpp>
#include <chrono>
#include <filesystem>
#include <gpiod.hpp>
#include <sstream>
#include <thread>
#include <utility>

#include "gpiosim.hpp"
#include "helpers.hpp"

using ::gpiosim::make_sim;
using direction = ::gpiod::line::direction;
using event_type = ::gpiod::info_event::event_type;

namespace {

void request_reconfigure_release_line(const ::std::filesystem::path& chip_path)
{
	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));

	auto request = ::gpiod::chip(chip_path)
		.prepare_request()
		.add_line_settings(7, ::gpiod::line_settings())
		.do_request();

	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));

	request.reconfigure_lines(
		::gpiod::line_config()
			.add_line_settings(
				7,
				::gpiod::line_settings()
					.set_direction(direction::OUTPUT)
			)
	);

	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));

	request.release();
}

TEST_CASE("Lines can be watched", "[info-event][chip]")
{
	auto sim = make_sim()
		.set_num_lines(8)
		.build();

	const auto chip_path = sim.dev_path();

	::gpiod::chip chip(chip_path);

	SECTION("watch_line_info() returns line info")
	{
		auto info = chip.watch_line_info(7);
		REQUIRE(info.offset() == 7);
	}

	SECTION("watch_line_info() fails for offset out of range")
	{
		REQUIRE_THROWS_AS(chip.watch_line_info(8), ::std::invalid_argument);
	}

	SECTION("waiting for event timeout")
	{
		chip.watch_line_info(3);
		REQUIRE_FALSE(chip.wait_info_event(::std::chrono::milliseconds(100)));
	}

	SECTION("request-reconfigure-release events")
	{
		auto info = chip.watch_line_info(7);
		::std::uint64_t ts_req, ts_rec, ts_rel;

		REQUIRE(info.direction() == direction::INPUT);

		::std::thread thread(request_reconfigure_release_line, ::std::ref(chip_path));

		REQUIRE(chip.wait_info_event(::std::chrono::seconds(1)));
		auto event = chip.read_info_event();
		REQUIRE(event.type() == event_type::LINE_REQUESTED);
		REQUIRE(event.get_line_info().direction() == direction::INPUT);
		ts_req = event.timestamp_ns();

		REQUIRE(chip.wait_info_event(::std::chrono::seconds(1)));
		event = chip.read_info_event();
		REQUIRE(event.type() == event_type::LINE_CONFIG_CHANGED);
		REQUIRE(event.get_line_info().direction() == direction::OUTPUT);
		ts_rec = event.timestamp_ns();

		REQUIRE(chip.wait_info_event(::std::chrono::seconds(1)));
		event = chip.read_info_event();
		REQUIRE(event.type() == event_type::LINE_RELEASED);
		ts_rel = event.timestamp_ns();

		/* No more events. */
		REQUIRE_FALSE(chip.wait_info_event(::std::chrono::milliseconds(100)));
		thread.join();

		/* Check timestamps are really monotonic. */
		REQUIRE(ts_rel > ts_rec);
		REQUIRE(ts_rec > ts_req);
	}
}

TEST_CASE("line info can be unwatched", "[info-event]")
{
	auto sim = make_sim()
		.set_num_lines(8)
		.build();

	::gpiod::chip chip(sim.dev_path());

	auto info = chip.watch_line_info(5);

	auto request = chip
		.prepare_request()
		.add_line_settings(5, ::gpiod::line_settings())
		.do_request();

	REQUIRE(chip.wait_info_event(::std::chrono::seconds(1)));
	auto event = chip.read_info_event();
	REQUIRE(event.type() == event_type::LINE_REQUESTED);

	chip.unwatch_line_info(5);

	request.release();

	REQUIRE_FALSE(chip.wait_info_event(::std::chrono::milliseconds(100)));
}

TEST_CASE("info_event can be copied and moved", "[info-event]")
{
	auto sim = make_sim().build();
	::gpiod::chip chip(sim.dev_path());
	::std::stringstream buf, expected;

	chip.watch_line_info(0);

	auto request = chip
		.prepare_request()
		.add_line_settings(0, ::gpiod::line_settings())
		.do_request();

	REQUIRE(chip.wait_info_event(::std::chrono::seconds(1)));
	auto event = chip.read_info_event();

	request.release();

	REQUIRE(chip.wait_info_event(::std::chrono::seconds(1)));
	auto copy = chip.read_info_event();

	SECTION("copy constructor works")
	{
		auto copy(event);

		REQUIRE(copy.type() == event_type::LINE_REQUESTED);
		REQUIRE(copy.get_line_info().offset() == 0);

		REQUIRE(event.type() == event_type::LINE_REQUESTED);
		REQUIRE(event.get_line_info().offset() == 0);
	}

	SECTION("assignment operator works")
	{
		copy = event;

		REQUIRE(copy.type() == event_type::LINE_REQUESTED);
		REQUIRE(copy.get_line_info().offset() == 0);

		REQUIRE(event.type() == event_type::LINE_REQUESTED);
		REQUIRE(event.get_line_info().offset() == 0);
	}

	SECTION("move constructor works")
	{
		auto copy(::std::move(event));

		REQUIRE(copy.type() == event_type::LINE_REQUESTED);
		REQUIRE(copy.get_line_info().offset() == 0);
	}

	SECTION("move assignment operator works")
	{
		copy = ::std::move(event);

		REQUIRE(copy.type() == event_type::LINE_REQUESTED);
		REQUIRE(copy.get_line_info().offset() == 0);
	}
}

TEST_CASE("info_event stream insertion operator works", "[info-event][line-info]")
{
	/*
	 * This tests the stream insertion operator for both the info_event
	 * and line_info classes.
	 */

	auto sim = make_sim().build();
	::gpiod::chip chip(sim.dev_path());
	::std::stringstream buf, expected;

	chip.watch_line_info(0);

	auto request = chip
		.prepare_request()
		.add_line_settings(0, ::gpiod::line_settings())
		.do_request();

	auto event = chip.read_info_event();

	buf << event;

	expected << "gpiod::info_event\\(event_type='LINE_REQUESTED', timestamp=[1-9][0-9]+, " <<
		    "line_info=gpiod::line_info\\(offset=0, name=unnamed, used=true, consumer='', " <<
		    "direction=INPUT, active_low=false, bias=UNKNOWN, drive=PUSH_PULL, " <<
		    "edge_detection=NONE, event_clock=MONOTONIC, debounced=false\\)\\)";

	REQUIRE_THAT(buf.str(), regex_matcher(expected.str()));
}

} /* namespace */
