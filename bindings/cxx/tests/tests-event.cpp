/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <catch.hpp>
#include <gpiod.hpp>
#include <map>
#include <poll.h>

#include "gpio-mockup.hpp"

using ::gpiod::test::mockup;

namespace {

const ::std::string consumer = "event-test";

} /* namespace */

TEST_CASE("Line events can be detected", "[event][line]")
{
	mockup::probe_guard mockup_chips({ 8 });
	mockup::event_thread events(0, 4, 200);
	::gpiod::chip chip(mockup::instance().chip_name(0));
	auto line = chip.get_line(4);
	::gpiod::line_request config;

	config.consumer = consumer.c_str();

	SECTION("rising edge")
	{
		config.request_type = ::gpiod::line_request::EVENT_RISING_EDGE;
		line.request(config);

		auto got_event = line.event_wait(::std::chrono::seconds(1));
		REQUIRE(got_event);

		auto event = line.event_read();
		REQUIRE(event.source == line);
		REQUIRE(event.event_type == ::gpiod::line_event::RISING_EDGE);
	}

	SECTION("falling edge")
	{
		config.request_type = ::gpiod::line_request::EVENT_FALLING_EDGE;

		line.request(config);

		auto got_event = line.event_wait(::std::chrono::seconds(1));
		REQUIRE(got_event);

		auto event = line.event_read();
		REQUIRE(event.source == line);
		REQUIRE(event.event_type == ::gpiod::line_event::FALLING_EDGE);
	}

	SECTION("both edges")
	{
		config.request_type = ::gpiod::line_request::EVENT_BOTH_EDGES;

		line.request(config);

		auto got_event = line.event_wait(::std::chrono::seconds(1));
		REQUIRE(got_event);

		auto event = line.event_read();
		REQUIRE(event.source == line);
		REQUIRE(event.event_type == ::gpiod::line_event::RISING_EDGE);

		got_event = line.event_wait(::std::chrono::seconds(1));
		REQUIRE(got_event);

		event = line.event_read();
		REQUIRE(event.source == line);
		REQUIRE(event.event_type == ::gpiod::line_event::FALLING_EDGE);
	}

	SECTION("active-low")
	{
		config.request_type = ::gpiod::line_request::EVENT_BOTH_EDGES;
		config.flags = ::gpiod::line_request::FLAG_ACTIVE_LOW;

		line.request(config);

		auto got_event = line.event_wait(::std::chrono::seconds(1));
		REQUIRE(got_event);

		auto event = line.event_read();
		REQUIRE(event.source == line);
		REQUIRE(event.event_type == ::gpiod::line_event::FALLING_EDGE);

		got_event = line.event_wait(::std::chrono::seconds(1));
		REQUIRE(got_event);

		event = line.event_read();
		REQUIRE(event.source == line);
		REQUIRE(event.event_type == ::gpiod::line_event::RISING_EDGE);
	}
}

TEST_CASE("Watching line_bulk objects for events works", "[event][bulk]")
{
	mockup::probe_guard mockup_chips({ 8 });
	mockup::event_thread events(0, 2, 200);
	::gpiod::chip chip(mockup::instance().chip_name(0));
	auto lines = chip.get_lines({ 0, 1, 2, 3 });
	::gpiod::line_request config;

	config.consumer = consumer.c_str();
	config.request_type = ::gpiod::line_request::EVENT_BOTH_EDGES;
	lines.request(config);

	auto event_lines = lines.event_wait(::std::chrono::seconds(1));
	REQUIRE(event_lines);
	REQUIRE(event_lines.size() == 1);

	auto event = event_lines.get(0).event_read();
	REQUIRE(event.source == event_lines.get(0));
	REQUIRE(event.event_type == ::gpiod::line_event::RISING_EDGE);

	event_lines = lines.event_wait(::std::chrono::seconds(1));
	REQUIRE(event_lines);
	REQUIRE(event_lines.size() == 1);

	event = event_lines.get(0).event_read();
	REQUIRE(event.source == event_lines.get(0));
	REQUIRE(event.event_type == ::gpiod::line_event::FALLING_EDGE);
}

TEST_CASE("It's possible to retrieve the event file descriptor", "[event][line]")
{
	mockup::probe_guard mockup_chips({ 8 });
	::gpiod::chip chip(mockup::instance().chip_name(0));
	auto line = chip.get_line(4);
	::gpiod::line_request config;

	config.consumer = consumer.c_str();

	SECTION("get the fd")
	{
		config.request_type = ::gpiod::line_request::EVENT_BOTH_EDGES;

		line.request(config);
		REQUIRE(line.event_get_fd() >= 0);
	}

	SECTION("error if not requested")
	{
		REQUIRE_THROWS_AS(line.event_get_fd(), ::std::system_error&);
	}

	SECTION("error if requested for values")
	{
		config.request_type = ::gpiod::line_request::DIRECTION_INPUT;

		line.request(config);
		REQUIRE_THROWS_AS(line.event_get_fd(), ::std::system_error&);
	}
}

TEST_CASE("Event file descriptors can be used for polling", "[event]")
{
	mockup::probe_guard mockup_chips({ 8 });
	mockup::event_thread events(0, 3, 200);
	::gpiod::chip chip(mockup::instance().chip_name(0));
	::std::map<int, ::gpiod::line> fd_line_map;
	auto lines = chip.get_lines({ 0, 1, 2, 3, 4, 5 });

	::gpiod::line_request config;
	config.consumer = consumer.c_str();
	config.request_type = ::gpiod::line_request::EVENT_BOTH_EDGES;

	lines.request(config);

	fd_line_map[lines[1].event_get_fd()] = lines[1];
	fd_line_map[lines[3].event_get_fd()] = lines[3];
	fd_line_map[lines[5].event_get_fd()] = lines[5];

	::pollfd fds[3];
	fds[0].fd = lines[1].event_get_fd();
	fds[1].fd = lines[3].event_get_fd();
	fds[2].fd = lines[5].event_get_fd();

	fds[0].events = fds[1].events = fds[2].events = POLLIN | POLLPRI;

	int ret = ::poll(fds, 3, 1000);
	REQUIRE(ret == 1);

	for (int i = 0; i < 3; i++) {
		if (fds[i].revents) {
			auto event = fd_line_map[fds[i].fd].event_read();
			REQUIRE(event.source == fd_line_map[fds[i].fd]);
			REQUIRE(event.event_type == ::gpiod::line_event::RISING_EDGE);
		}
	}
}

TEST_CASE("It's possible to read values from lines requested for events", "[event][line]")
{
	mockup::probe_guard mockup_chips({ 8 });
	::gpiod::chip chip(mockup::instance().chip_name(0));
	auto line = chip.get_line(4);
	::gpiod::line_request config;

	config.consumer = consumer.c_str();
	config.request_type = ::gpiod::line_request::EVENT_BOTH_EDGES;

	mockup::instance().chip_set_pull(0, 4, 1);

	SECTION("active-high (default)")
	{
		line.request(config);
		REQUIRE(line.get_value() == 1);
	}

	SECTION("active-low")
	{
		config.flags = ::gpiod::line_request::FLAG_ACTIVE_LOW;
		line.request(config);
		REQUIRE(line.get_value() == 0);
	}
}
