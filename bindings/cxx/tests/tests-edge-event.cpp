// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <catch2/catch_all.hpp>
#include <chrono>
#include <gpiod.hpp>
#include <sstream>
#include <thread>
#include <utility>

#include "gpiosim.hpp"
#include "helpers.hpp"

using ::gpiosim::make_sim;
using direction = ::gpiod::line::direction;
using edge = ::gpiod::line::edge;
using offsets = ::gpiod::line::offsets;
using pull = ::gpiosim::chip::pull;
using event_type = ::gpiod::edge_event::event_type;

namespace {

TEST_CASE("edge_event_buffer capacity settings work", "[edge-event]")
{
	SECTION("default capacity")
	{
		REQUIRE(::gpiod::edge_event_buffer().capacity() == 64);
	}

	SECTION("user-defined capacity")
	{
		REQUIRE(::gpiod::edge_event_buffer(123).capacity() == 123);
	}

	SECTION("max capacity")
	{
		REQUIRE(::gpiod::edge_event_buffer(16 * 64 * 2).capacity() == 1024);
	}
}

TEST_CASE("edge_event wait timeout", "[edge-event]")
{
	auto sim = make_sim().build();
	::gpiod::chip chip(sim.dev_path());

	auto request = chip.prepare_request()
		.add_line_settings(
			0,
			::gpiod::line_settings()
				.set_edge_detection(edge::BOTH)
		)
		.do_request();

	REQUIRE_FALSE(request.wait_edge_events(::std::chrono::milliseconds(100)));
}

TEST_CASE("output mode and edge detection don't work together", "[edge-event]")
{
	auto sim = make_sim().build();

	REQUIRE_THROWS_AS(
		::gpiod::chip(sim.dev_path())
			.prepare_request()
			.add_line_settings(
				0,
				::gpiod::line_settings()
					.set_direction(direction::OUTPUT)
					.set_edge_detection(edge::BOTH)
			)
			.do_request(),
		::std::invalid_argument
	);
}

void trigger_falling_and_rising_edge(::gpiosim::chip& sim, unsigned int offset)
{
	::std::this_thread::sleep_for(::std::chrono::milliseconds(30));
	sim.set_pull(offset, pull::PULL_UP);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(30));
	sim.set_pull(offset, pull::PULL_DOWN);
}

void trigger_rising_edge_events_on_two_offsets(::gpiosim::chip& sim,
					       unsigned int off0, unsigned int off1)
{
	::std::this_thread::sleep_for(::std::chrono::milliseconds(30));
	sim.set_pull(off0, pull::PULL_UP);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(30));
	sim.set_pull(off1, pull::PULL_UP);
}

TEST_CASE("waiting for and reading edge events works", "[edge-event]")
{
	auto sim = make_sim()
		.set_num_lines(8)
		.build();

	::gpiod::chip chip(sim.dev_path());
	::gpiod::edge_event_buffer buffer;

	SECTION("both edge events")
	{
		auto request = chip
			.prepare_request()
			.add_line_settings(
				2,
				::gpiod::line_settings()
					.set_edge_detection(edge::BOTH)
			)
			.do_request();

		::std::uint64_t ts_rising, ts_falling;

		::std::thread thread(trigger_falling_and_rising_edge, ::std::ref(sim), 2);

		REQUIRE(request.wait_edge_events(::std::chrono::seconds(1)));
		REQUIRE(request.read_edge_events(buffer, 1) == 1);
		REQUIRE(buffer.num_events() == 1);
		auto event = buffer.get_event(0);
		REQUIRE(event.type() == event_type::RISING_EDGE);
		REQUIRE(event.line_offset() == 2);
		ts_rising = event.timestamp_ns();

		REQUIRE(request.wait_edge_events(::std::chrono::seconds(1)));
		REQUIRE(request.read_edge_events(buffer, 1) == 1);
		REQUIRE(buffer.num_events() == 1);
		event = buffer.get_event(0);
		REQUIRE(event.type() == event_type::FALLING_EDGE);
		REQUIRE(event.line_offset() == 2);
		ts_falling = event.timestamp_ns();

		REQUIRE_FALSE(request.wait_edge_events(::std::chrono::milliseconds(100)));

		thread.join();

		REQUIRE(ts_falling > ts_rising);
	}

	SECTION("rising edge event")
	{
		auto request = chip
			.prepare_request()
			.add_line_settings(
				6,
				::gpiod::line_settings()
					.set_edge_detection(edge::RISING)
			)
			.do_request();

		::std::thread thread(trigger_falling_and_rising_edge, ::std::ref(sim), 6);

		REQUIRE(request.wait_edge_events(::std::chrono::seconds(1)));
		REQUIRE(request.read_edge_events(buffer, 1) == 1);
		REQUIRE(buffer.num_events() == 1);
		auto event = buffer.get_event(0);
		REQUIRE(event.type() == event_type::RISING_EDGE);
		REQUIRE(event.line_offset() == 6);

		REQUIRE_FALSE(request.wait_edge_events(::std::chrono::milliseconds(100)));

		thread.join();
	}

	SECTION("falling edge event")
	{
		auto request = chip
			.prepare_request()
			.add_line_settings(
				7,
				::gpiod::line_settings()
					.set_edge_detection(edge::FALLING)
			)
			.do_request();

		::std::thread thread(trigger_falling_and_rising_edge, ::std::ref(sim), 7);

		REQUIRE(request.wait_edge_events(::std::chrono::seconds(1)));
		REQUIRE(request.read_edge_events(buffer, 1) == 1);
		REQUIRE(buffer.num_events() == 1);
		auto event = buffer.get_event(0);
		REQUIRE(event.type() == event_type::FALLING_EDGE);
		REQUIRE(event.line_offset() == 7);

		REQUIRE_FALSE(request.wait_edge_events(::std::chrono::milliseconds(100)));

		thread.join();
	}

	SECTION("sequence numbers")
	{
		auto request = chip
			.prepare_request()
			.add_line_settings(
				{ 0, 1 },
				::gpiod::line_settings()
					.set_edge_detection(edge::BOTH)
			)
			.do_request();

		::std::thread thread(trigger_rising_edge_events_on_two_offsets, ::std::ref(sim), 0, 1);

		REQUIRE(request.wait_edge_events(::std::chrono::seconds(1)));
		REQUIRE(request.read_edge_events(buffer, 1) == 1);
		REQUIRE(buffer.num_events() == 1);
		auto event = buffer.get_event(0);
		REQUIRE(event.type() == event_type::RISING_EDGE);
		REQUIRE(event.line_offset() == 0);
		REQUIRE(event.global_seqno() == 1);
		REQUIRE(event.line_seqno() == 1);

		REQUIRE(request.wait_edge_events(::std::chrono::seconds(1)));
		REQUIRE(request.read_edge_events(buffer, 1) == 1);
		REQUIRE(buffer.num_events() == 1);
		event = buffer.get_event(0);
		REQUIRE(event.type() == event_type::RISING_EDGE);
		REQUIRE(event.line_offset() == 1);
		REQUIRE(event.global_seqno() == 2);
		REQUIRE(event.line_seqno() == 1);

		thread.join();
	}
}

TEST_CASE("reading multiple events", "[edge-event]")
{
	auto sim = make_sim()
		.set_num_lines(8)
		.build();

	::gpiod::chip chip(sim.dev_path());

	auto request = chip
		.prepare_request()
		.add_line_settings(
			1,
			::gpiod::line_settings()
				.set_edge_detection(edge::BOTH)
		)
		.do_request();

	unsigned long line_seqno = 1, global_seqno = 1;

	sim.set_pull(1, pull::PULL_UP);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
	sim.set_pull(1, pull::PULL_DOWN);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
	sim.set_pull(1, pull::PULL_UP);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));

	SECTION("read multiple events")
	{
		::gpiod::edge_event_buffer buffer;

		REQUIRE(request.wait_edge_events(::std::chrono::seconds(1)));
		REQUIRE(request.read_edge_events(buffer) == 3);
		REQUIRE(buffer.num_events() == 3);

		for (const auto& event: buffer) {
			REQUIRE(event.line_offset() == 1);
			REQUIRE(event.line_seqno() == line_seqno++);
			REQUIRE(event.global_seqno() == global_seqno++);
		}
	}

	SECTION("read over capacity")
	{
		::gpiod::edge_event_buffer buffer(2);

		REQUIRE(request.wait_edge_events(::std::chrono::seconds(1)));
		REQUIRE(request.read_edge_events(buffer) == 2);
		REQUIRE(buffer.num_events() == 2);
	}
}

TEST_CASE("edge_event_buffer can be moved", "[edge-event]")
{
	auto sim = make_sim()
		.set_num_lines(2)
		.build();

	::gpiod::chip chip(sim.dev_path());
	::gpiod::edge_event_buffer buffer(13);

	/* Get some events into the buffer. */
	auto request = chip
		.prepare_request()
		.add_line_settings(
			1,
			::gpiod::line_settings()
				.set_edge_detection(edge::BOTH)
		)
		.do_request();

	sim.set_pull(1, pull::PULL_UP);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
	sim.set_pull(1, pull::PULL_DOWN);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
	sim.set_pull(1, pull::PULL_UP);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));

	::std::this_thread::sleep_for(::std::chrono::milliseconds(500));

	REQUIRE(request.wait_edge_events(::std::chrono::seconds(1)));
	REQUIRE(request.read_edge_events(buffer) == 3);

	SECTION("move constructor works")
	{
		auto moved(::std::move(buffer));
		REQUIRE(moved.capacity() == 13);
		REQUIRE(moved.num_events() == 3);
	}

	SECTION("move assignment operator works")
	{
		::gpiod::edge_event_buffer moved;

		moved = ::std::move(buffer);
		REQUIRE(moved.capacity() == 13);
		REQUIRE(moved.num_events() == 3);
	}
}

TEST_CASE("edge_event can be copied and moved", "[edge-event]")
{
	auto sim = make_sim().build();
	::gpiod::chip chip(sim.dev_path());
	::gpiod::edge_event_buffer buffer;

	auto request = chip
		.prepare_request()
		.add_line_settings(
			0,
			::gpiod::line_settings()
				.set_edge_detection(edge::BOTH)
		)
		.do_request();

	sim.set_pull(0, pull::PULL_UP);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
	REQUIRE(request.wait_edge_events(::std::chrono::seconds(1)));
	REQUIRE(request.read_edge_events(buffer) == 1);
	auto event = buffer.get_event(0);

	sim.set_pull(0, pull::PULL_DOWN);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
	REQUIRE(request.wait_edge_events(::std::chrono::seconds(1)));
	REQUIRE(request.read_edge_events(buffer) == 1);
	auto copy = buffer.get_event(0);

	SECTION("copy constructor works")
	{
		auto copy(event);
		REQUIRE(copy.line_offset() == 0);
		REQUIRE(copy.type() == event_type::RISING_EDGE);
		REQUIRE(event.line_offset() == 0);
		REQUIRE(event.type() == event_type::RISING_EDGE);
	}

	SECTION("move constructor works")
	{
		auto copy(::std::move(event));
		REQUIRE(copy.line_offset() == 0);
		REQUIRE(copy.type() == event_type::RISING_EDGE);
	}

	SECTION("assignment operator works")
	{
		copy = event;
		REQUIRE(copy.line_offset() == 0);
		REQUIRE(copy.type() == event_type::RISING_EDGE);
		REQUIRE(event.line_offset() == 0);
		REQUIRE(event.type() == event_type::RISING_EDGE);
	}

	SECTION("move assignment operator works")
	{
		copy = ::std::move(event);
		REQUIRE(copy.line_offset() == 0);
		REQUIRE(copy.type() == event_type::RISING_EDGE);
	}
}

TEST_CASE("stream insertion operators work for edge_event and edge_event_buffer", "[edge-event]")
{
	/*
	 * This tests the stream insertion operators for both edge_event and
	 * edge_event_buffer classes.
	 */

	auto sim = make_sim().build();
	::gpiod::chip chip(sim.dev_path());
	::gpiod::edge_event_buffer buffer;
	::std::stringstream sbuf, expected;

	auto request = chip
		.prepare_request()
		.add_line_settings(
			0,
			::gpiod::line_settings()
				.set_edge_detection(edge::BOTH)
		)
		.do_request();

	sim.set_pull(0, pull::PULL_UP);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(30));
	sim.set_pull(0, pull::PULL_DOWN);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(30));

	REQUIRE(request.wait_edge_events(::std::chrono::seconds(1)));
	REQUIRE(request.read_edge_events(buffer) == 2);

	sbuf << buffer;

	expected << "gpiod::edge_event_buffer\\(num_events=2, capacity=64, events=\\[gpiod::edge_event\\" <<
		    "(type='RISING_EDGE', timestamp=[1-9][0-9]+, line_offset=0, global_seqno=1, " <<
		    "line_seqno=1\\), gpiod::edge_event\\(type='FALLING_EDGE', timestamp=[1-9][0-9]+, " <<
		    "line_offset=0, global_seqno=2, line_seqno=2\\)\\]\\)";

	REQUIRE_THAT(sbuf.str(), regex_matcher(expected.str()));
}

} /* namespace */
