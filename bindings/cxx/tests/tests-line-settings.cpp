// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <catch2/catch.hpp>
#include <gpiod.hpp>

#include "helpers.hpp"

using value = ::gpiod::line::value;
using direction = ::gpiod::line::direction;
using edge = ::gpiod::line::edge;
using bias = ::gpiod::line::bias;
using drive = ::gpiod::line::drive;
using clock_type = ::gpiod::line::clock;
using value = ::gpiod::line::value;

using namespace ::std::chrono_literals;

namespace {

TEST_CASE("line_settings constructor works", "[line-settings]")
{
	::gpiod::line_settings settings;

	REQUIRE(settings.direction() == direction::AS_IS);
	REQUIRE(settings.edge_detection() == edge::NONE);
	REQUIRE(settings.bias() == bias::AS_IS);
	REQUIRE(settings.drive() == drive::PUSH_PULL);
	REQUIRE_FALSE(settings.active_low());
	REQUIRE(settings.debounce_period() == 0us);
	REQUIRE(settings.event_clock() == clock_type::MONOTONIC);
	REQUIRE(settings.output_value() == value::INACTIVE);
}

TEST_CASE("line_settings mutators work", "[line-settings]")
{
	::gpiod::line_settings settings;

	SECTION("direction")
	{
		settings.set_direction(direction::INPUT);
		REQUIRE(settings.direction() == direction::INPUT);
		settings.set_direction(direction::AS_IS);
		REQUIRE(settings.direction() == direction::AS_IS);
		settings.set_direction(direction::OUTPUT);
		REQUIRE(settings.direction() == direction::OUTPUT);
		REQUIRE_THROWS_AS(settings.set_direction(static_cast<direction>(999)),
				  ::std::invalid_argument);
	}

	SECTION("edge detection")
	{
		settings.set_edge_detection(edge::BOTH);
		REQUIRE(settings.edge_detection() == edge::BOTH);
		settings.set_edge_detection(edge::NONE);
		REQUIRE(settings.edge_detection() == edge::NONE);
		settings.set_edge_detection(edge::FALLING);
		REQUIRE(settings.edge_detection() == edge::FALLING);
		settings.set_edge_detection(edge::RISING);
		REQUIRE(settings.edge_detection() == edge::RISING);
		REQUIRE_THROWS_AS(settings.set_edge_detection(static_cast<edge>(999)),
				  ::std::invalid_argument);
	}

	SECTION("bias")
	{
		settings.set_bias(bias::DISABLED);
		REQUIRE(settings.bias() == bias::DISABLED);
		settings.set_bias(bias::AS_IS);
		REQUIRE(settings.bias() == bias::AS_IS);
		settings.set_bias(bias::PULL_DOWN);
		REQUIRE(settings.bias() == bias::PULL_DOWN);
		settings.set_bias(bias::PULL_UP);
		REQUIRE(settings.bias() == bias::PULL_UP);
		REQUIRE_THROWS_AS(settings.set_bias(static_cast<bias>(999)), ::std::invalid_argument);
		REQUIRE_THROWS_AS(settings.set_bias(bias::UNKNOWN), ::std::invalid_argument);
	}

	SECTION("drive")
	{
		settings.set_drive(drive::OPEN_DRAIN);
		REQUIRE(settings.drive() == drive::OPEN_DRAIN);
		settings.set_drive(drive::PUSH_PULL);
		REQUIRE(settings.drive() == drive::PUSH_PULL);
		settings.set_drive(drive::OPEN_SOURCE);
		REQUIRE(settings.drive() == drive::OPEN_SOURCE);
		REQUIRE_THROWS_AS(settings.set_drive(static_cast<drive>(999)), ::std::invalid_argument);
	}

	SECTION("active-low")
	{
		settings.set_active_low(true);
		REQUIRE(settings.active_low());
		settings.set_active_low(false);
		REQUIRE_FALSE(settings.active_low());
	}

	SECTION("debounce period")
	{
		settings.set_debounce_period(2000us);
		REQUIRE(settings.debounce_period() == 2000us);
	}

	SECTION("event clock")
	{
		settings.set_event_clock(clock_type::REALTIME);
		REQUIRE(settings.event_clock() == clock_type::REALTIME);
		settings.set_event_clock(clock_type::MONOTONIC);
		REQUIRE(settings.event_clock() == clock_type::MONOTONIC);
		settings.set_event_clock(clock_type::HTE);
		REQUIRE(settings.event_clock() == clock_type::HTE);
		REQUIRE_THROWS_AS(settings.set_event_clock(static_cast<clock_type>(999)),
				  ::std::invalid_argument);
	}

	SECTION("output value")
	{
		settings.set_output_value(value::ACTIVE);
		REQUIRE(settings.output_value() == value::ACTIVE);
		settings.set_output_value(value::INACTIVE);
		REQUIRE(settings.output_value() == value::INACTIVE);
		REQUIRE_THROWS_AS(settings.set_output_value(static_cast<value>(999)),
				  ::std::invalid_argument);
	}
}

TEST_CASE("line_settings can be moved and copied", "[line-settings]")
{
	::gpiod::line_settings settings;

	settings
		.set_direction(direction::INPUT)
		.set_edge_detection(edge::BOTH);

	SECTION("copy constructor works")
	{
		auto copy(settings);
		settings.set_direction(direction::OUTPUT);
		settings.set_edge_detection(edge::NONE);
		REQUIRE(copy.direction() == direction::INPUT);
		REQUIRE(copy.edge_detection() == edge::BOTH);
	}

	SECTION("assignment operator works")
	{
		::gpiod::line_settings copy;
		copy = settings;
		settings.set_direction(direction::OUTPUT);
		settings.set_edge_detection(edge::NONE);
		REQUIRE(copy.direction() == direction::INPUT);
		REQUIRE(copy.edge_detection() == edge::BOTH);
	}

	SECTION("move constructor works")
	{
		auto copy(::std::move(settings));
		REQUIRE(copy.direction() == direction::INPUT);
		REQUIRE(copy.edge_detection() == edge::BOTH);
	}

	SECTION("move assignment operator works")
	{
		::gpiod::line_settings copy;
		copy = ::std::move(settings);
		REQUIRE(copy.direction() == direction::INPUT);
		REQUIRE(copy.edge_detection() == edge::BOTH);
	}
}

TEST_CASE("line_settings stream insertion operator works", "[line-settings]")
{
	::gpiod::line_settings settings;

	REQUIRE_THAT(settings
		.set_active_low(true)
		.set_direction(direction::INPUT)
		.set_edge_detection(edge::BOTH)
		.set_bias(bias::PULL_DOWN)
		.set_event_clock(clock_type::REALTIME),
		stringify_matcher<::gpiod::line_settings>(
			"gpiod::line_settings(direction=INPUT, edge_detection=BOTH_EDGES, "
			"bias=PULL_DOWN, drive=PUSH_PULL, active-low, debounce_period=0, "
			"event_clock=REALTIME, output_value=INACTIVE)"
		)
	);
}

} /* namespace */
