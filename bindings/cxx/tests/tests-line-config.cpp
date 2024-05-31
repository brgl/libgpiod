// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <catch2/catch_all.hpp>
#include <gpiod.hpp>

#include "gpiosim.hpp"
#include "helpers.hpp"

using ::gpiosim::make_sim;
using namespace ::std::chrono_literals;
using direction = ::gpiod::line::direction;
using drive = ::gpiod::line::drive;
using edge = ::gpiod::line::edge;
using simval = ::gpiosim::chip::value;
using value = ::gpiod::line::value;
using values = ::gpiod::line::values;

namespace {

TEST_CASE("line_config constructor works", "[line-config]")
{
	SECTION("no arguments - default values")
	{
		::gpiod::line_config cfg;

		REQUIRE(cfg.get_line_settings().size() == 0);
	}
}

TEST_CASE("adding line_settings to line_config works", "[line-config][line-settings]")
{
	::gpiod::line_config cfg;

	cfg.add_line_settings(4,
		::gpiod::line_settings()
			.set_direction(direction::INPUT)
			.set_edge_detection(edge::RISING));

	cfg.add_line_settings({7, 2},
		::gpiod::line_settings()
			.set_direction(direction::OUTPUT)
			.set_drive(drive::OPEN_DRAIN));

	auto settings = cfg.get_line_settings();

	REQUIRE(settings.size() == 3);
	REQUIRE(settings.at(2).direction() == direction::OUTPUT);
	REQUIRE(settings.at(2).drive() == drive::OPEN_DRAIN);
	REQUIRE(settings.at(4).direction() == direction::INPUT);
	REQUIRE(settings.at(4).edge_detection() == edge::RISING);
	REQUIRE(settings.at(7).direction() == direction::OUTPUT);
	REQUIRE(settings.at(7).drive() == drive::OPEN_DRAIN);
}

TEST_CASE("line_config can be reset", "[line-config]")
{
	::gpiod::line_config cfg;

	cfg.add_line_settings({3, 4, 7},
		::gpiod::line_settings()
			.set_direction(direction::INPUT)
			.set_edge_detection(edge::BOTH));

	auto settings = cfg.get_line_settings();

	REQUIRE(settings.size() == 3);
	REQUIRE(settings.at(3).direction() == direction::INPUT);
	REQUIRE(settings.at(3).edge_detection() == edge::BOTH);
	REQUIRE(settings.at(4).direction() == direction::INPUT);
	REQUIRE(settings.at(4).edge_detection() == edge::BOTH);
	REQUIRE(settings.at(7).direction() == direction::INPUT);
	REQUIRE(settings.at(7).edge_detection() == edge::BOTH);

	cfg.reset();

	REQUIRE(cfg.get_line_settings().size() == 0);
}

TEST_CASE("output values can be set globally", "[line-config]")
{
	const values vals = { value::ACTIVE, value::INACTIVE, value::ACTIVE, value::INACTIVE };

	auto sim = make_sim()
		.set_num_lines(4)
		.build();

	::gpiod::line_config cfg;

	SECTION("request with globally set output values")
	{
		cfg
			.add_line_settings(
				{0, 1, 2, 3},
				::gpiod::line_settings().set_direction(direction::OUTPUT)
			)
			.set_output_values(vals);

		auto request = ::gpiod::chip(sim.dev_path())
			.prepare_request()
			.set_line_config(cfg)
			.do_request();

		REQUIRE(sim.get_value(0) == simval::ACTIVE);
		REQUIRE(sim.get_value(1) == simval::INACTIVE);
		REQUIRE(sim.get_value(2) == simval::ACTIVE);
		REQUIRE(sim.get_value(3) == simval::INACTIVE);
	}

	SECTION("read back global output values")
	{
		cfg
			.add_line_settings(
				{0, 1, 2, 3},
				::gpiod::line_settings()
					.set_direction(direction::OUTPUT)
					.set_output_value(value::ACTIVE)
			)
			.set_output_values(vals);

		auto settings = cfg.get_line_settings()[1];
		REQUIRE(settings.output_value() == value::INACTIVE);
	}
}

TEST_CASE("line_config stream insertion operator works", "[line-config]")
{
	::gpiod::line_config cfg;

	SECTION("empty config")
	{
		REQUIRE_THAT(cfg, stringify_matcher<::gpiod::line_config>(
					"gpiod::line_config(num_settings=0)"));
	}

	SECTION("config with settings")
	{
		cfg.add_line_settings({0, 2},
				::gpiod::line_settings()
					.set_direction(direction::OUTPUT)
					.set_drive(drive::OPEN_SOURCE)
		);

		REQUIRE_THAT(cfg, stringify_matcher<::gpiod::line_config>(
			"gpiod::line_config(num_settings=2, "
			"settings=[0: gpiod::line_settings(direction=OUTPUT, edge_detection=NONE, "
			"bias=AS_IS, drive=OPEN_SOURCE, active-high, debounce_period=0, "
			"event_clock=MONOTONIC, output_value=INACTIVE), "
			"2: gpiod::line_settings(direction=OUTPUT, edge_detection=NONE, bias=AS_IS, "
			"drive=OPEN_SOURCE, active-high, debounce_period=0, event_clock=MONOTONIC, "
			"output_value=INACTIVE)])"));
	}
}

} /* namespace */
