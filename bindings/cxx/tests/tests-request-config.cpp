// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2021-2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <catch2/catch_all.hpp>
#include <cstddef>
#include <gpiod.hpp>
#include <string>
#include <sstream>

#include "helpers.hpp"

using offsets = ::gpiod::line::offsets;

namespace {

TEST_CASE("request_config constructor works", "[request-config]")
{
	SECTION("no arguments")
	{
		::gpiod::request_config cfg;

		REQUIRE(cfg.consumer().empty());
		REQUIRE(cfg.event_buffer_size() == 0);
	}
}

TEST_CASE("request_config can be moved", "[request-config]")
{
	::gpiod::request_config cfg;

	cfg.set_consumer("foobar").set_event_buffer_size(64);

	SECTION("move constructor works")
	{
		auto moved(::std::move(cfg));
		REQUIRE_THAT(moved.consumer(), Catch::Matchers::Equals("foobar"));
		REQUIRE(moved.event_buffer_size() == 64);
	}

	SECTION("move assignment operator works")
	{
		::gpiod::request_config moved;

		moved = ::std::move(cfg);

		REQUIRE_THAT(moved.consumer(), Catch::Matchers::Equals("foobar"));
		REQUIRE(moved.event_buffer_size() == 64);
	}
}

TEST_CASE("request_config mutators work", "[request-config]")
{
	::gpiod::request_config cfg;

	SECTION("set consumer")
	{
		cfg.set_consumer("foobar");
		REQUIRE_THAT(cfg.consumer(), Catch::Matchers::Equals("foobar"));
	}

	SECTION("set event_buffer_size")
	{
		cfg.set_event_buffer_size(128);
		REQUIRE(cfg.event_buffer_size() == 128);
	}
}

TEST_CASE("request_config stream insertion operator works", "[request-config]")
{
	::gpiod::request_config cfg;

	cfg.set_consumer("foobar").set_event_buffer_size(32);

	::std::stringstream buf;

	buf << cfg;

	::std::string expected("gpiod::request_config(consumer='foobar', event_buffer_size=32)");

	REQUIRE_THAT(buf.str(), Catch::Matchers::Equals(expected));
}

} /* namespace */
