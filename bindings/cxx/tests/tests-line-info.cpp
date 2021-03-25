// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <catch2/catch.hpp>
#include <gpiod.hpp>
#include <string>

#include "helpers.hpp"
#include "gpiosim.hpp"

using ::gpiosim::make_sim;
using hog_dir = ::gpiosim::chip_builder::hog_direction;
using direction = ::gpiod::line::direction;
using edge = ::gpiod::line::edge;
using bias = ::gpiod::line::bias;
using drive = ::gpiod::line::drive;
using event_clock = ::gpiod::line::clock;

using namespace ::std::chrono_literals;

namespace {

TEST_CASE("get_line_info() works", "[chip][line-info]")
{
	auto sim = make_sim()
		.set_num_lines(8)
		.set_line_name(0,  "foobar")
		.set_hog(0, "hog", hog_dir::OUTPUT_HIGH)
		.build();

	::gpiod::chip chip(sim.dev_path());

	SECTION("line_info can be retrieved from chip")
	{
		auto info = chip.get_line_info(0);

		REQUIRE(info.offset() == 0);
		REQUIRE_THAT(info.name(), Catch::Equals("foobar"));
		REQUIRE(info.used());
		REQUIRE_THAT(info.consumer(), Catch::Equals("hog"));
		REQUIRE(info.direction() == ::gpiod::line::direction::OUTPUT);
		REQUIRE_FALSE(info.active_low());
		REQUIRE(info.bias() == ::gpiod::line::bias::UNKNOWN);
		REQUIRE(info.drive() == ::gpiod::line::drive::PUSH_PULL);
		REQUIRE(info.edge_detection() == ::gpiod::line::edge::NONE);
		REQUIRE(info.event_clock() == ::gpiod::line::clock::MONOTONIC);
		REQUIRE_FALSE(info.debounced());
		REQUIRE(info.debounce_period() == 0us);
	}

	SECTION("offset out of range")
	{
		REQUIRE_THROWS_AS(chip.get_line_info(8), ::std::invalid_argument);
	}
}

TEST_CASE("line properties can be retrieved", "[line-info]")
{
	auto sim = make_sim()
		.set_num_lines(8)
		.set_line_name(1, "foo")
		.set_line_name(2, "bar")
		.set_line_name(4, "baz")
		.set_line_name(5, "xyz")
		.set_hog(3, "hog3", hog_dir::OUTPUT_HIGH)
		.set_hog(4, "hog4", hog_dir::OUTPUT_LOW)
		.build();

	::gpiod::chip chip(sim.dev_path());

	SECTION("basic properties")
	{
		auto info4 = chip.get_line_info(4);
		auto info6 = chip.get_line_info(6);

		REQUIRE(info4.offset() == 4);
		REQUIRE_THAT(info4.name(), Catch::Equals("baz"));
		REQUIRE(info4.used());
		REQUIRE_THAT(info4.consumer(), Catch::Equals("hog4"));
		REQUIRE(info4.direction() == direction::OUTPUT);
		REQUIRE(info4.edge_detection() == edge::NONE);
		REQUIRE_FALSE(info4.active_low());
		REQUIRE(info4.bias() == bias::UNKNOWN);
		REQUIRE(info4.drive() == drive::PUSH_PULL);
		REQUIRE(info4.event_clock() == event_clock::MONOTONIC);
		REQUIRE_FALSE(info4.debounced());
		REQUIRE(info4.debounce_period() == 0us);
	}
}

TEST_CASE("line_info can be copied and moved")
{
	auto sim = make_sim()
		.set_num_lines(4)
		.set_line_name(2, "foobar")
		.build();

	::gpiod::chip chip(sim.dev_path());
	auto info = chip.get_line_info(2);

	SECTION("copy constructor works")
	{
		auto copy(info);
		REQUIRE(copy.offset() == 2);
		REQUIRE_THAT(copy.name(), Catch::Equals("foobar"));
		/* info can still be used */
		REQUIRE(info.offset() == 2);
		REQUIRE_THAT(info.name(), Catch::Equals("foobar"));
	}

	SECTION("assignment operator works")
	{
		auto copy = chip.get_line_info(0);
		copy = info;
		REQUIRE(copy.offset() == 2);
		REQUIRE_THAT(copy.name(), Catch::Equals("foobar"));
		/* info can still be used */
		REQUIRE(info.offset() == 2);
		REQUIRE_THAT(info.name(), Catch::Equals("foobar"));
	}

	SECTION("move constructor works")
	{
		auto copy(::std::move(info));
		REQUIRE(copy.offset() == 2);
		REQUIRE_THAT(copy.name(), Catch::Equals("foobar"));
	}

	SECTION("move assignment operator works")
	{
		auto copy = chip.get_line_info(0);
		copy = ::std::move(info);
		REQUIRE(copy.offset() == 2);
		REQUIRE_THAT(copy.name(), Catch::Equals("foobar"));
	}
}

TEST_CASE("line_info stream insertion operator works")
{
	auto sim = make_sim()
		.set_line_name(0, "foo")
		.set_hog(0, "hogger", hog_dir::OUTPUT_HIGH)
		.build();

	::gpiod::chip chip(sim.dev_path());

	auto info = chip.get_line_info(0);

	REQUIRE_THAT(info, stringify_matcher<::gpiod::line_info>(
		"gpiod::line_info(offset=0, name='foo', used=true, consumer='foo', direction=OUTPUT, "
		"active_low=false, bias=UNKNOWN, drive=PUSH_PULL, edge_detection=NONE, event_clock=MONOTONIC, debounced=false)"));
}

} /* namespace */
