/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <catch2/catch.hpp>
#include <gpiod.hpp>

#include "gpio-mockup.hpp"

using ::gpiod::test::mockup;

namespace {

const ::std::string consumer = "line-test";

} /* namespace */

TEST_CASE("Global find_line() function works", "[line]")
{
	mockup::probe_guard mockup_chips({ 8, 8, 8, 8, 8 }, mockup::FLAG_NAMED_LINES);

	SECTION("line found")
	{
		auto line = ::gpiod::find_line("gpio-mockup-C-5");
		REQUIRE(line.offset() == 5);
		REQUIRE(line.name() == "gpio-mockup-C-5");
		REQUIRE(line.get_chip().label() == "gpio-mockup-C");
	}

	SECTION("line not found")
	{
		auto line = ::gpiod::find_line("nonexistent-line");
		REQUIRE_FALSE(line);
	}
}

TEST_CASE("Line information can be correctly retrieved", "[line]")
{
	mockup::probe_guard mockup_chips({ 8 }, mockup::FLAG_NAMED_LINES);
	::gpiod::chip chip(mockup::instance().chip_name(0));
	auto line = chip.get_line(4);

	SECTION("unexported line")
	{
		REQUIRE(line.offset() == 4);
		REQUIRE(line.name() == "gpio-mockup-A-4");
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_INPUT);
		REQUIRE(line.active_state() == ::gpiod::line::ACTIVE_HIGH);
		REQUIRE(line.consumer().empty());
		REQUIRE_FALSE(line.is_requested());
		REQUIRE_FALSE(line.is_used());
		REQUIRE_FALSE(line.is_open_drain());
		REQUIRE_FALSE(line.is_open_source());
		REQUIRE(line.bias() == ::gpiod::line::BIAS_AS_IS);
	}

	SECTION("exported line")
	{
		::gpiod::line_request config;

		config.consumer = consumer.c_str();
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		line.request(config);

		REQUIRE(line.offset() == 4);
		REQUIRE(line.name() == "gpio-mockup-A-4");
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(line.active_state() == ::gpiod::line::ACTIVE_HIGH);
		REQUIRE(line.is_requested());
		REQUIRE(line.is_used());
		REQUIRE_FALSE(line.is_open_drain());
		REQUIRE_FALSE(line.is_open_source());
		REQUIRE(line.bias() == ::gpiod::line::BIAS_AS_IS);
	}

	SECTION("exported line with flags")
	{
		::gpiod::line_request config;

		config.consumer = consumer.c_str();
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		config.flags = ::gpiod::line_request::FLAG_ACTIVE_LOW |
			       ::gpiod::line_request::FLAG_OPEN_DRAIN;
		line.request(config);

		REQUIRE(line.offset() == 4);
		REQUIRE(line.name() == "gpio-mockup-A-4");
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(line.active_state() == ::gpiod::line::ACTIVE_LOW);
		REQUIRE(line.is_requested());
		REQUIRE(line.is_used());
		REQUIRE(line.is_open_drain());
		REQUIRE_FALSE(line.is_open_source());
		REQUIRE(line.bias() == ::gpiod::line::BIAS_AS_IS);
	}

	SECTION("exported open source line")
	{
		::gpiod::line_request config;

		config.consumer = consumer.c_str();
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		config.flags = ::gpiod::line_request::FLAG_OPEN_SOURCE;
		line.request(config);

		REQUIRE(line.offset() == 4);
		REQUIRE(line.name() == "gpio-mockup-A-4");
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(line.active_state() == ::gpiod::line::ACTIVE_HIGH);
		REQUIRE(line.is_requested());
		REQUIRE(line.is_used());
		REQUIRE_FALSE(line.is_open_drain());
		REQUIRE(line.is_open_source());
		REQUIRE(line.bias() == ::gpiod::line::BIAS_AS_IS);
	}

	SECTION("exported bias disable line")
	{
		::gpiod::line_request config;

		config.consumer = consumer.c_str();
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		config.flags = ::gpiod::line_request::FLAG_BIAS_DISABLE;
		line.request(config);

		REQUIRE(line.offset() == 4);
		REQUIRE(line.name() == "gpio-mockup-A-4");
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(line.active_state() == ::gpiod::line::ACTIVE_HIGH);
		REQUIRE(line.is_requested());
		REQUIRE(line.is_used());
		REQUIRE_FALSE(line.is_open_drain());
		REQUIRE_FALSE(line.is_open_source());
		REQUIRE(line.bias() == ::gpiod::line::BIAS_DISABLE);
	}

	SECTION("exported pull-down line")
	{
		::gpiod::line_request config;

		config.consumer = consumer.c_str();
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		config.flags = ::gpiod::line_request::FLAG_BIAS_PULL_DOWN;
		line.request(config);

		REQUIRE(line.offset() == 4);
		REQUIRE(line.name() == "gpio-mockup-A-4");
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(line.active_state() == ::gpiod::line::ACTIVE_HIGH);
		REQUIRE(line.is_requested());
		REQUIRE(line.is_used());
		REQUIRE_FALSE(line.is_open_drain());
		REQUIRE_FALSE(line.is_open_source());
		REQUIRE(line.bias() == ::gpiod::line::BIAS_PULL_DOWN);
	}

	SECTION("exported pull-up line")
	{
		::gpiod::line_request config;

		config.consumer = consumer.c_str();
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		config.flags = ::gpiod::line_request::FLAG_BIAS_PULL_UP;
		line.request(config);

		REQUIRE(line.offset() == 4);
		REQUIRE(line.name() == "gpio-mockup-A-4");
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(line.active_state() == ::gpiod::line::ACTIVE_HIGH);
		REQUIRE(line.is_requested());
		REQUIRE(line.is_used());
		REQUIRE_FALSE(line.is_open_drain());
		REQUIRE_FALSE(line.is_open_source());
		REQUIRE(line.bias() == ::gpiod::line::BIAS_PULL_UP);
	}

	SECTION("update line info")
	{
		REQUIRE_NOTHROW(line.update());
	}
}

TEST_CASE("Line bulk object works correctly", "[line][bulk]")
{
	mockup::probe_guard mockup_chips({ 8 }, mockup::FLAG_NAMED_LINES);
	::gpiod::chip chip(mockup::instance().chip_name(0));

	SECTION("lines can be added, accessed and cleared")
	{
		::gpiod::line_bulk lines;

		REQUIRE(lines.empty());

		lines.append(chip.get_line(0));
		lines.append(chip.get_line(1));
		lines.append(chip.get_line(2));

		REQUIRE(lines.size() == 3);
		REQUIRE(lines.get(1).name() == "gpio-mockup-A-1");
		REQUIRE(lines[2].name() == "gpio-mockup-A-2");

		lines.clear();

		REQUIRE(lines.empty());
	}

	SECTION("bulk iterator works")
	{
		auto lines = chip.get_all_lines();
		int count = 0;

		for (auto& it: lines)
			REQUIRE(it.offset() == count++);

		REQUIRE(count == chip.num_lines());
	}

	SECTION("accessing lines out of range throws exception")
	{
		auto lines = chip.get_all_lines();

		REQUIRE_THROWS_AS(lines.get(11), ::std::out_of_range);
	}
}

TEST_CASE("Line values can be set and read", "[line]")
{
	mockup::probe_guard mockup_chips({ 8 });
	::gpiod::chip chip(mockup::instance().chip_name(0));
	::gpiod::line_request config;

	config.consumer = consumer.c_str();

	SECTION("get value (single line)")
	{
		auto line = chip.get_line(3);
		config.request_type = ::gpiod::line_request::DIRECTION_INPUT;
		line.request(config);
		REQUIRE(line.get_value() == 0);
		mockup::instance().chip_set_pull(0, 3, 1);
		REQUIRE(line.get_value() == 1);
	}

	SECTION("set value (single line)")
	{
		auto line = chip.get_line(3);
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		line.request(config);
		line.set_value(1);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 1);
		line.set_value(0);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 0);
	}

	SECTION("set value with default value parameter")
	{
		auto line = chip.get_line(3);
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		line.request(config, 1);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 1);
	}

	SECTION("get multiple values at once")
	{
		auto lines = chip.get_lines({ 0, 1, 2, 3, 4 });
		config.request_type = ::gpiod::line_request::DIRECTION_INPUT;
		lines.request(config);
		REQUIRE(lines.get_values() == ::std::vector<int>({ 0, 0, 0, 0, 0 }));
		mockup::instance().chip_set_pull(0, 1, 1);
		mockup::instance().chip_set_pull(0, 3, 1);
		mockup::instance().chip_set_pull(0, 4, 1);
		REQUIRE(lines.get_values() == ::std::vector<int>({ 0, 1, 0, 1, 1 }));
	}

	SECTION("set multiple values at once")
	{
		auto lines = chip.get_lines({ 0, 1, 2, 6, 7 });
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		lines.request(config);
		lines.set_values({ 1, 1, 0, 1, 0 });
		REQUIRE(mockup::instance().chip_get_value(0, 0) == 1);
		REQUIRE(mockup::instance().chip_get_value(0, 1) == 1);
		REQUIRE(mockup::instance().chip_get_value(0, 2) == 0);
		REQUIRE(mockup::instance().chip_get_value(0, 6) == 1);
		REQUIRE(mockup::instance().chip_get_value(0, 7) == 0);
	}

	SECTION("set multiple values with default values parameter")
	{
		auto lines = chip.get_lines({ 1, 2, 4, 6, 7 });
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		lines.request(config, { 1, 1, 0, 1, 0 });
		REQUIRE(mockup::instance().chip_get_value(0, 1) == 1);
		REQUIRE(mockup::instance().chip_get_value(0, 2) == 1);
		REQUIRE(mockup::instance().chip_get_value(0, 4) == 0);
		REQUIRE(mockup::instance().chip_get_value(0, 6) == 1);
		REQUIRE(mockup::instance().chip_get_value(0, 7) == 0);
	}

	SECTION("get value (single line, active-low")
	{
		auto line = chip.get_line(4);
		config.request_type = ::gpiod::line_request::DIRECTION_INPUT;
		config.flags = ::gpiod::line_request::FLAG_ACTIVE_LOW;
		line.request(config);
		REQUIRE(line.get_value() == 1);
		mockup::instance().chip_set_pull(0, 4, 1);
		REQUIRE(line.get_value() == 0);
	}

	SECTION("set value (single line, active-low)")
	{
		auto line = chip.get_line(3);
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		config.flags = ::gpiod::line_request::FLAG_ACTIVE_LOW;
		line.request(config);
		line.set_value(1);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 0);
		line.set_value(0);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 1);
	}
}

TEST_CASE("Line can be reconfigured", "[line]")
{
	mockup::probe_guard mockup_chips({ 8 });
	::gpiod::chip chip(mockup::instance().chip_name(0));
	::gpiod::line_request config;

	config.consumer = consumer.c_str();

	SECTION("set config (single line, active-state)")
	{
		auto line = chip.get_line(3);
		config.request_type = ::gpiod::line_request::DIRECTION_INPUT;
		config.flags = 0;
		line.request(config);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_INPUT);
		REQUIRE(line.active_state() == ::gpiod::line::ACTIVE_HIGH);

		line.set_config(::gpiod::line_request::DIRECTION_OUTPUT,
			::gpiod::line_request::FLAG_ACTIVE_LOW,1);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(line.active_state() == ::gpiod::line::ACTIVE_LOW);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 0);
		line.set_value(0);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 1);

		line.set_config(::gpiod::line_request::DIRECTION_OUTPUT, 0);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(line.active_state() == ::gpiod::line::ACTIVE_HIGH);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 0);
		line.set_value(1);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 1);
	}

	SECTION("set flags (single line, active-state)")
	{
		auto line = chip.get_line(3);
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		config.flags = 0;
		line.request(config,1);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 1);

		line.set_flags(::gpiod::line_request::FLAG_ACTIVE_LOW);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(line.active_state() == ::gpiod::line::ACTIVE_LOW);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 0);

		line.set_flags(0);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(line.active_state() == ::gpiod::line::ACTIVE_HIGH);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 1);
	}

	SECTION("set flags (single line, drive)")
	{
		auto line = chip.get_line(3);
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		config.flags = 0;
		line.request(config);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE_FALSE(line.is_open_drain());
		REQUIRE_FALSE(line.is_open_source());

		line.set_flags(::gpiod::line_request::FLAG_OPEN_DRAIN);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(line.is_open_drain());
		REQUIRE_FALSE(line.is_open_source());

		line.set_flags(::gpiod::line_request::FLAG_OPEN_SOURCE);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE_FALSE(line.is_open_drain());
		REQUIRE(line.is_open_source());

		line.set_flags(0);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE_FALSE(line.is_open_drain());
		REQUIRE_FALSE(line.is_open_source());
	}

	SECTION("set flags (single line, bias)")
	{
		auto line = chip.get_line(3);
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		config.flags = 0;
		line.request(config);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE_FALSE(line.is_open_drain());
		REQUIRE_FALSE(line.is_open_source());

		line.set_flags(::gpiod::line_request::FLAG_OPEN_DRAIN);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(line.is_open_drain());
		REQUIRE_FALSE(line.is_open_source());

		line.set_flags(::gpiod::line_request::FLAG_OPEN_SOURCE);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE_FALSE(line.is_open_drain());
		REQUIRE(line.is_open_source());

		line.set_flags(0);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE_FALSE(line.is_open_drain());
		REQUIRE_FALSE(line.is_open_source());
	}

	SECTION("set direction input (single line)")
	{
		auto line = chip.get_line(3);
		config.request_type = ::gpiod::line_request::DIRECTION_OUTPUT;
		config.flags = 0;
		line.request(config);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		line.set_direction_input();
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_INPUT);
	}

	SECTION("set direction output (single line)")
	{
		auto line = chip.get_line(3);
		config.request_type = ::gpiod::line_request::DIRECTION_INPUT;
		config.flags = 0;
		line.request(config);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_INPUT);
		line.set_direction_output(1);
		REQUIRE(line.direction() == ::gpiod::line::DIRECTION_OUTPUT);
		REQUIRE(mockup::instance().chip_get_value(0, 3) == 1);
	}
}

TEST_CASE("Exported line can be released", "[line]")
{
	mockup::probe_guard mockup_chips({ 8 });
	::gpiod::chip chip(mockup::instance().chip_name(0));
	auto line = chip.get_line(4);
	::gpiod::line_request config;

	config.consumer = consumer.c_str();
	config.request_type = ::gpiod::line_request::DIRECTION_INPUT;

	line.request(config);

	REQUIRE(line.is_requested());
	REQUIRE(line.get_value() == 0);

	line.release();

	REQUIRE_FALSE(line.is_requested());
	REQUIRE_THROWS_AS(line.get_value(), ::std::system_error);
}

TEST_CASE("Uninitialized GPIO line behaves correctly", "[line]")
{
	::gpiod::line line;

	SECTION("uninitialized line is 'false'")
	{
		REQUIRE_FALSE(line);
	}

	SECTION("using uninitialized line throws logic_error")
	{
		REQUIRE_THROWS_AS(line.name(), ::std::logic_error);
	}
}

TEST_CASE("Uninitialized GPIO line_bulk behaves correctly", "[line][bulk]")
{
	::gpiod::line_bulk bulk;

	SECTION("uninitialized line_bulk is 'false'")
	{
		REQUIRE_FALSE(bulk);
	}

	SECTION("using uninitialized line_bulk throws logic_error")
	{
		REQUIRE_THROWS_AS(bulk.get(0), ::std::logic_error);
	}
}

TEST_CASE("Cannot request the same line twice", "[line]")
{
	mockup::probe_guard mockup_chips({ 8 });
	::gpiod::chip chip(mockup::instance().chip_name(0));
	::gpiod::line_request config;

	config.consumer = consumer.c_str();
	config.request_type = ::gpiod::line_request::DIRECTION_INPUT;

	SECTION("two separate calls to request()")
	{
		auto line = chip.get_line(3);

		REQUIRE_NOTHROW(line.request(config));
		REQUIRE_THROWS_AS(line.request(config), ::std::system_error);
	}

	SECTION("request the same line twice in line_bulk")
	{
		/*
		 * While a line_bulk object can hold two or more line objects
		 * representing the same line - requesting it will fail.
		 */
		auto lines = chip.get_lines({ 2, 3, 4, 4 });

		REQUIRE_THROWS_AS(lines.request(config), ::std::system_error);
	}
}

TEST_CASE("Cannot get/set values of unrequested lines", "[line]")
{
	mockup::probe_guard mockup_chips({ 8 });
	::gpiod::chip chip(mockup::instance().chip_name(0));
	auto line = chip.get_line(3);

	SECTION("get value")
	{
		REQUIRE_THROWS_AS(line.get_value(), ::std::system_error);
	}

	SECTION("set value")
	{
		REQUIRE_THROWS_AS(line.set_value(1), ::std::system_error);
	}
}

TEST_CASE("Line objects can be compared")
{
	mockup::probe_guard mockup_chips({ 8 });
	::gpiod::chip chip(mockup::instance().chip_name(0));
	auto line1 = chip.get_line(3);
	auto line2 = chip.get_line(3);
	auto line3 = chip.get_line(4);

	REQUIRE(line1 == line2);
	REQUIRE(line2 != line3);
}
