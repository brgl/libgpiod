// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <catch2/catch.hpp>
#include <gpiod.hpp>

#include "helpers.hpp"

using offset = ::gpiod::line::offset;
using value = ::gpiod::line::value;
using direction = ::gpiod::line::direction;
using edge = ::gpiod::line::edge;
using bias = ::gpiod::line::bias;
using drive = ::gpiod::line::drive;
using clock_type = ::gpiod::line::clock;
using offsets = ::gpiod::line::offsets;
using values = ::gpiod::line::values;
using value_mapping = ::gpiod::line::value_mapping;
using value_mappings = ::gpiod::line::value_mappings;

namespace {

TEST_CASE("stream insertion operators for types in gpiod::line work", "[line]")
{
	SECTION("offset")
	{
		offset off = 4;

		REQUIRE_THAT(off, stringify_matcher<offset>("4"));
	}

	SECTION("value")
	{
		auto active = value::ACTIVE;
		auto inactive = value::INACTIVE;

		REQUIRE_THAT(active, stringify_matcher<value>("ACTIVE"));
		REQUIRE_THAT(inactive, stringify_matcher<value>("INACTIVE"));
	}

	SECTION("direction")
	{
		auto input = direction::INPUT;
		auto output = direction::OUTPUT;
		auto as_is = direction::AS_IS;

		REQUIRE_THAT(input, stringify_matcher<direction>("INPUT"));
		REQUIRE_THAT(output, stringify_matcher<direction>("OUTPUT"));
		REQUIRE_THAT(as_is, stringify_matcher<direction>("AS_IS"));
	}

	SECTION("edge")
	{
		auto rising = edge::RISING;
		auto falling = edge::FALLING;
		auto both = edge::BOTH;
		auto none = edge::NONE;

		REQUIRE_THAT(rising, stringify_matcher<edge>("RISING_EDGE"));
		REQUIRE_THAT(falling, stringify_matcher<edge>("FALLING_EDGE"));
		REQUIRE_THAT(both, stringify_matcher<edge>("BOTH_EDGES"));
		REQUIRE_THAT(none, stringify_matcher<edge>("NONE"));
	}

	SECTION("bias")
	{
		auto pull_up = bias::PULL_UP;
		auto pull_down = bias::PULL_DOWN;
		auto disabled = bias::DISABLED;
		auto as_is = bias::AS_IS;
		auto unknown = bias::UNKNOWN;

		REQUIRE_THAT(pull_up, stringify_matcher<bias>("PULL_UP"));
		REQUIRE_THAT(pull_down, stringify_matcher<bias>("PULL_DOWN"));
		REQUIRE_THAT(disabled, stringify_matcher<bias>("DISABLED"));
		REQUIRE_THAT(as_is, stringify_matcher<bias>("AS_IS"));
		REQUIRE_THAT(unknown, stringify_matcher<bias>("UNKNOWN"));
	}

	SECTION("drive")
	{
		auto push_pull = drive::PUSH_PULL;
		auto open_drain = drive::OPEN_DRAIN;
		auto open_source = drive::OPEN_SOURCE;

		REQUIRE_THAT(push_pull, stringify_matcher<drive>("PUSH_PULL"));
		REQUIRE_THAT(open_drain, stringify_matcher<drive>("OPEN_DRAIN"));
		REQUIRE_THAT(open_source, stringify_matcher<drive>("OPEN_SOURCE"));
	}

	SECTION("clock")
	{
		auto monotonic = clock_type::MONOTONIC;
		auto realtime = clock_type::REALTIME;
		auto hte = clock_type::HTE;

		REQUIRE_THAT(monotonic, stringify_matcher<clock_type>("MONOTONIC"));
		REQUIRE_THAT(realtime, stringify_matcher<clock_type>("REALTIME"));
		REQUIRE_THAT(hte, stringify_matcher<clock_type>("HTE"));
	}

	SECTION("offsets")
	{
		offsets offs = { 2, 5, 3, 9, 8, 7 };

		REQUIRE_THAT(offs, stringify_matcher<offsets>("gpiod::offsets(2, 5, 3, 9, 8, 7)"));
	}

	SECTION("values")
	{
		values vals = {
			value::ACTIVE,
			value::INACTIVE,
			value::ACTIVE,
			value::ACTIVE,
			value::INACTIVE
		};

		REQUIRE_THAT(vals,
			     stringify_matcher<values>("gpiod::values(ACTIVE, INACTIVE, ACTIVE, ACTIVE, INACTIVE)"));
	}

	SECTION("value_mapping")
	{
		value_mapping val = { 4, value::ACTIVE };

		REQUIRE_THAT(val, stringify_matcher<value_mapping>("gpiod::value_mapping(4: ACTIVE)"));
	}

	SECTION("value_mappings")
	{
		value_mappings vals = { { 0, value::ACTIVE }, { 4, value::INACTIVE }, { 8, value::ACTIVE } };

		REQUIRE_THAT(vals, stringify_matcher<value_mappings>(
			"gpiod::value_mappings(gpiod::value_mapping(0: ACTIVE), gpiod::value_mapping(4: INACTIVE), gpiod::value_mapping(8: ACTIVE))"));
	}
}

} /* namespace */
