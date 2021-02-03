// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <catch2/catch.hpp>
#include <gpiod.hpp>
#include <poll.h>
#include <thread>

#include "gpio-mockup.hpp"

using ::gpiod::test::mockup;

namespace {

const ::std::string consumer = "event-test";

} /* namespace */

TEST_CASE("Line events can be detected", "[event][line]")
{
	mockup::probe_guard mockup_chips({ 8 });
	mockup::event_thread events(0, 4, 200);
	::gpiod::chip chip(mockup::instance().chip_path(0));
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
	::gpiod::chip chip(mockup::instance().chip_path(0));
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
	::gpiod::chip chip(mockup::instance().chip_path(0));
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
		REQUIRE_THROWS_AS(line.event_get_fd(), ::std::system_error);
	}

	SECTION("error if requested for values")
	{
		config.request_type = ::gpiod::line_request::DIRECTION_INPUT;

		line.request(config);
		REQUIRE_THROWS_AS(line.event_get_fd(), ::std::system_error);
	}
}

TEST_CASE("Event file descriptors can be used for polling", "[event]")
{
	mockup::probe_guard mockup_chips({ 8 });
	mockup::event_thread events(0, 3, 200);
	::gpiod::chip chip(mockup::instance().chip_path(0));
	auto lines = chip.get_lines({ 0, 1, 2, 3, 4, 5 });

	::gpiod::line_request config;
	config.consumer = consumer.c_str();
	config.request_type = ::gpiod::line_request::EVENT_BOTH_EDGES;

	lines.request(config);

	::std::vector<::pollfd> fds(3);
	fds[0].fd = lines[1].event_get_fd();
	fds[1].fd = lines[3].event_get_fd();
	fds[2].fd = lines[5].event_get_fd();

	fds[0].events = fds[1].events = fds[2].events = POLLIN | POLLPRI;

	int ret = ::poll(fds.data(), 3, 1000);
	REQUIRE(ret == 1);

	for (int i = 0; i < 3; i++) {
		if (fds[i].revents) {
			auto event = lines[3].event_read();
			REQUIRE(event.source == lines[3]);
			REQUIRE(event.event_type == ::gpiod::line_event::RISING_EDGE);
		}
	}
}

TEST_CASE("It's possible to read a value from a line requested for events", "[event][line]")
{
	mockup::probe_guard mockup_chips({ 8 });
	::gpiod::chip chip(mockup::instance().chip_path(0));
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

TEST_CASE("It's possible to read values from lines requested for events", "[event][bulk]")
{
	mockup::probe_guard mockup_chips({ 8 });
	::gpiod::chip chip(mockup::instance().chip_path(0));
	auto lines = chip.get_lines({ 0, 1, 2, 3, 4 });
	::gpiod::line_request config;

	config.consumer = consumer.c_str();
	config.request_type = ::gpiod::line_request::EVENT_BOTH_EDGES;

	mockup::instance().chip_set_pull(0, 5, 1);

	SECTION("active-high (default)")
	{
		lines.request(config);
		REQUIRE(lines.get_values() == ::std::vector<int>({ 0, 0, 0, 0, 0 }));
		mockup::instance().chip_set_pull(0, 1, 1);
		mockup::instance().chip_set_pull(0, 3, 1);
		mockup::instance().chip_set_pull(0, 4, 1);
		REQUIRE(lines.get_values() == ::std::vector<int>({ 0, 1, 0, 1, 1 }));
	}

	SECTION("active-low")
	{
		config.flags = ::gpiod::line_request::FLAG_ACTIVE_LOW;
		lines.request(config);
		REQUIRE(lines.get_values() == ::std::vector<int>({ 1, 1, 1, 1, 1 }));
		mockup::instance().chip_set_pull(0, 1, 1);
		mockup::instance().chip_set_pull(0, 3, 1);
		mockup::instance().chip_set_pull(0, 4, 1);
		REQUIRE(lines.get_values() == ::std::vector<int>({ 1, 0, 1, 0, 0 }));
	}
}

TEST_CASE("It's possible to read more than one line event", "[event][line]")
{
	mockup::probe_guard mockup_chips({ 8 });
	::gpiod::chip chip(mockup::instance().chip_path(0));
	auto line = chip.get_line(4);
	::gpiod::line_request config;

	config.consumer = consumer.c_str();
	config.request_type = ::gpiod::line_request::EVENT_BOTH_EDGES;

	line.request(config);

	mockup::instance().chip_set_pull(0, 4, 1);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
	mockup::instance().chip_set_pull(0, 4, 0);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));
	mockup::instance().chip_set_pull(0, 4, 1);
	::std::this_thread::sleep_for(::std::chrono::milliseconds(10));

	auto events = line.event_read_multiple();

	REQUIRE(events.size() == 3);
	REQUIRE(events.at(0).event_type == ::gpiod::line_event::RISING_EDGE);
	REQUIRE(events.at(1).event_type == ::gpiod::line_event::FALLING_EDGE);
	REQUIRE(events.at(2).event_type == ::gpiod::line_event::RISING_EDGE);
	REQUIRE(events.at(0).source == line);
	REQUIRE(events.at(1).source == line);
	REQUIRE(events.at(2).source == line);
}
