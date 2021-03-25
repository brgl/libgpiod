// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <catch2/catch.hpp>
#include <gpiod.hpp>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "gpiosim.hpp"
#include "helpers.hpp"

using ::gpiosim::make_sim;
using offsets = ::gpiod::line::offsets;
using values = ::gpiod::line::values;
using direction = ::gpiod::line::direction;
using value = ::gpiod::line::value;
using simval = ::gpiosim::chip::value;
using pull = ::gpiosim::chip::pull;

namespace {

class value_matcher : public Catch::MatcherBase<value>
{
public:
	value_matcher(pull pull, bool active_low = false)
		: _m_pull(pull),
		  _m_active_low(active_low)
	{

	}

	::std::string describe() const override
	{
		::std::string repr(this->_m_pull == pull::PULL_UP ? "PULL_UP" : "PULL_DOWN");
		::std::string active_low = this->_m_active_low ? "(active-low) " : "";

		return active_low + "corresponds with " + repr;
	}

	bool match(const value& val) const override
	{
		if (this->_m_active_low) {
			if ((val == value::ACTIVE && this->_m_pull == pull::PULL_DOWN) ||
			    (val == value::INACTIVE && this->_m_pull == pull::PULL_UP))
				return true;
		} else {
			if ((val == value::ACTIVE && this->_m_pull == pull::PULL_UP) ||
			    (val == value::INACTIVE && this->_m_pull == pull::PULL_DOWN))
				return true;
		}

		return false;
	}

private:
	pull _m_pull;
	bool _m_active_low;
};

TEST_CASE("requesting lines behaves correctly with invalid arguments", "[line-request][chip]")
{
	auto sim = make_sim()
		.set_num_lines(8)
		.build();

	::gpiod::chip chip(sim.dev_path());

	SECTION("no offsets")
	{
		REQUIRE_THROWS_AS(chip.prepare_request().do_request(), ::std::invalid_argument);
	}

	SECTION("duplicate offsets")
	{
		auto request = chip
			.prepare_request()
			.add_line_settings({ 2, 0, 0, 4 }, ::gpiod::line_settings())
			.do_request();

		auto offsets = request.offsets();

		REQUIRE(offsets.size() == 3);
		REQUIRE(offsets[0] == 2);
		REQUIRE(offsets[1] == 0);
		REQUIRE(offsets[2] == 4);
	}

	SECTION("offset out of bounds")
	{
		REQUIRE_THROWS_AS(chip
			.prepare_request()
			.add_line_settings({ 2, 0, 8, 4 }, ::gpiod::line_settings())
			.do_request(),
			::std::invalid_argument
		);
	}
}

TEST_CASE("consumer string is set correctly", "[line-request]")
{
	auto sim = make_sim()
		.set_num_lines(4)
		.build();

	::gpiod::chip chip(sim.dev_path());
	offsets offs({ 3, 0, 2 });

	SECTION("set custom consumer")
	{
		auto request = chip
			.prepare_request()
			.add_line_settings(offs, ::gpiod::line_settings())
			.set_consumer("foobar")
			.do_request();

		auto info = chip.get_line_info(2);

		REQUIRE(info.used());
		REQUIRE_THAT(info.consumer(), Catch::Equals("foobar"));
	}

	SECTION("empty consumer")
	{
		auto request = chip
			.prepare_request()
			.add_line_settings(2, ::gpiod::line_settings())
			.do_request();

		auto info = chip.get_line_info(2);

		REQUIRE(info.used());
		REQUIRE_THAT(info.consumer(), Catch::Equals("?"));
	}
}

TEST_CASE("values can be read", "[line-request]")
{
	auto sim = make_sim()
		.set_num_lines(8)
		.build();

	const offsets offs({ 7, 1, 0, 6, 2 });

	const ::std::vector<pull> pulls({
		pull::PULL_UP,
		pull::PULL_UP,
		pull::PULL_DOWN,
		pull::PULL_UP,
		pull::PULL_DOWN
	});

	for (unsigned int i = 0; i < offs.size(); i++)
		sim.set_pull(offs[i], pulls[i]);

	auto request = ::gpiod::chip(sim.dev_path())
		.prepare_request()
		.add_line_settings(
			offs,
			::gpiod::line_settings()
				.set_direction(direction::INPUT)
		)
		.do_request();

	SECTION("get all values (returning variant)")
	{
		auto vals = request.get_values();

		REQUIRE_THAT(vals[0], value_matcher(pull::PULL_UP));
		REQUIRE_THAT(vals[1], value_matcher(pull::PULL_UP));
		REQUIRE_THAT(vals[2], value_matcher(pull::PULL_DOWN));
		REQUIRE_THAT(vals[3], value_matcher(pull::PULL_UP));
		REQUIRE_THAT(vals[4], value_matcher(pull::PULL_DOWN));
	}

	SECTION("get all values (passed buffer variant)")
	{
		values vals(5);

		request.get_values(vals);

		REQUIRE_THAT(vals[0], value_matcher(pull::PULL_UP));
		REQUIRE_THAT(vals[1], value_matcher(pull::PULL_UP));
		REQUIRE_THAT(vals[2], value_matcher(pull::PULL_DOWN));
		REQUIRE_THAT(vals[3], value_matcher(pull::PULL_UP));
		REQUIRE_THAT(vals[4], value_matcher(pull::PULL_DOWN));
	}

	SECTION("get_values(buffer) throws for invalid buffer size")
	{
		values vals(4);
		REQUIRE_THROWS_AS(request.get_values(vals), ::std::invalid_argument);
		vals.resize(6);
		REQUIRE_THROWS_AS(request.get_values(vals), ::std::invalid_argument);
	}

	SECTION("get a single value")
	{
		auto val = request.get_value(7);

		REQUIRE_THAT(val, value_matcher(pull::PULL_UP));
	}

	SECTION("get a single value (active-low)")
	{
		request.reconfigure_lines(
			::gpiod::line_config()
				.add_line_settings(
					offs,
					::gpiod::line_settings()
						.set_active_low(true))
		);

		auto val = request.get_value(7);

		REQUIRE_THAT(val, value_matcher(pull::PULL_UP, true));
	}

	SECTION("get a subset of values (returning variant)")
	{
		auto vals = request.get_values(offsets({ 2, 0, 6 }));

		REQUIRE_THAT(vals[0], value_matcher(pull::PULL_DOWN));
		REQUIRE_THAT(vals[1], value_matcher(pull::PULL_DOWN));
		REQUIRE_THAT(vals[2], value_matcher(pull::PULL_UP));
	}

	SECTION("get a subset of values (passed buffer variant)")
	{
		values vals(3);

		request.get_values(offsets({ 2, 0, 6 }), vals);

		REQUIRE_THAT(vals[0], value_matcher(pull::PULL_DOWN));
		REQUIRE_THAT(vals[1], value_matcher(pull::PULL_DOWN));
		REQUIRE_THAT(vals[2], value_matcher(pull::PULL_UP));
	}
}

TEST_CASE("output values can be set at request time", "[line-request]")
{
	auto sim = make_sim()
		.set_num_lines(8)
		.build();

	::gpiod::chip chip(sim.dev_path());
	const offsets offs({ 0, 1, 3, 4 });

	::gpiod::line_settings settings;
	settings.set_direction(direction::OUTPUT);
	settings.set_output_value(value::ACTIVE);

	::gpiod::line_config line_cfg;
	line_cfg.add_line_settings(offs, settings);

	SECTION("default output value")
	{
		auto request = chip
			.prepare_request()
			.set_line_config(line_cfg)
			.do_request();

		for (const auto& off: offs)
			REQUIRE(sim.get_value(off) == simval::ACTIVE);

		REQUIRE(sim.get_value(2) == simval::INACTIVE);
	}

	SECTION("overridden output value")
	{
		settings.set_output_value(value::INACTIVE);
		line_cfg.add_line_settings(1, settings);

		auto request = chip
			.prepare_request()
			.set_line_config(line_cfg)
			.do_request();

		REQUIRE(sim.get_value(0) == simval::ACTIVE);
		REQUIRE(sim.get_value(1) == simval::INACTIVE);
		REQUIRE(sim.get_value(2) == simval::INACTIVE);
		REQUIRE(sim.get_value(3) == simval::ACTIVE);
		REQUIRE(sim.get_value(4) == simval::ACTIVE);
	}
}

TEST_CASE("values can be set after requesting lines", "[line-request]")
{
	auto sim = make_sim()
		.set_num_lines(8)
		.build();

	const offsets offs({ 0, 1, 3, 4 });

	auto request = ::gpiod::chip(sim.dev_path())
		.prepare_request()
		.add_line_settings(
			offs,
			::gpiod::line_settings()
				.set_direction(direction::OUTPUT)
		)
		.do_request();

	SECTION("set single value")
	{
		request.set_value(1, value::ACTIVE);

		REQUIRE(sim.get_value(0) == simval::INACTIVE);
		REQUIRE(sim.get_value(1) == simval::ACTIVE);
		REQUIRE(sim.get_value(3) == simval::INACTIVE);
		REQUIRE(sim.get_value(4) == simval::INACTIVE);
	}

	SECTION("set all values")
	{
		request.set_values({
			value::ACTIVE,
			value::INACTIVE,
			value::ACTIVE,
			value::INACTIVE
		});

		REQUIRE(sim.get_value(0) == simval::ACTIVE);
		REQUIRE(sim.get_value(1) == simval::INACTIVE);
		REQUIRE(sim.get_value(3) == simval::ACTIVE);
		REQUIRE(sim.get_value(4) == simval::INACTIVE);
	}

	SECTION("set a subset of values")
	{
		request.set_values({ 4, 3 }, { value::ACTIVE, value::INACTIVE });

		REQUIRE(sim.get_value(0) == simval::INACTIVE);
		REQUIRE(sim.get_value(1) == simval::INACTIVE);
		REQUIRE(sim.get_value(3) == simval::INACTIVE);
		REQUIRE(sim.get_value(4) == simval::ACTIVE);
	}

	SECTION("set a subset of values with mappings")
	{
		request.set_values({
			{ 0, value::ACTIVE },
			{ 4, value::INACTIVE },
			{ 1, value::ACTIVE}
		});

		REQUIRE(sim.get_value(0) == simval::ACTIVE);
		REQUIRE(sim.get_value(1) == simval::ACTIVE);
		REQUIRE(sim.get_value(3) == simval::INACTIVE);
		REQUIRE(sim.get_value(4) == simval::INACTIVE);
	}
}

TEST_CASE("line_request can be moved", "[line-request]")
{
	auto sim = make_sim()
		.set_num_lines(8)
		.build();

	::gpiod::chip chip(sim.dev_path());
	const offsets offs({ 3, 1, 0, 2 });

	auto request = chip
		.prepare_request()
		.add_line_settings(
			offs,
			::gpiod::line_settings()
		)
		.do_request();

	auto fd = request.fd();

	auto another = chip
		.prepare_request()
		.add_line_settings(6, ::gpiod::line_settings())
		.do_request();

	SECTION("move constructor works")
	{
		auto moved(::std::move(request));

		REQUIRE(moved.fd() == fd);
		REQUIRE_THAT(moved.offsets(), Catch::Equals(offs));
	}

	SECTION("move assignment operator works")
	{
		another = ::std::move(request);

		REQUIRE(another.fd() == fd);
		REQUIRE_THAT(another.offsets(), Catch::Equals(offs));
	}
}

TEST_CASE("released request can no longer be used", "[line-request]")
{
	auto sim = make_sim().build();

	auto request = ::gpiod::chip(sim.dev_path())
		.prepare_request()
		.add_line_settings(0, ::gpiod::line_settings())
		.do_request();

	request.release();

	REQUIRE_THROWS_AS(request.offsets(), ::gpiod::request_released);
}

TEST_CASE("line_request survives parent chip", "[line-request][chip]")
{
	auto sim = make_sim().build();

	sim.set_pull(0, pull::PULL_UP);

	SECTION("chip is released")
	{
		::gpiod::chip chip(sim.dev_path());

		auto request = chip
			.prepare_request()
			.add_line_settings(
				0,
				::gpiod::line_settings()
					.set_direction(direction::INPUT)
			)
			.do_request();

		REQUIRE_THAT(request.get_value(0), value_matcher(pull::PULL_UP));

		chip.close();

		REQUIRE_THAT(request.get_value(0), value_matcher(pull::PULL_UP));
	}

	SECTION("chip goes out of scope")
	{
		/* Need to get the request object somehow. */
		::gpiod::chip dummy(sim.dev_path());
		::gpiod::line_config cfg;
		cfg.add_line_settings(0, ::gpiod::line_settings().set_direction(direction::INPUT));

		auto request = dummy
			.prepare_request()
			.set_line_config(cfg)
			.do_request();

		request.release();
		dummy.close();

		{
			::gpiod::chip chip(sim.dev_path());

			request = chip
				.prepare_request()
				.set_line_config(cfg)
				.do_request();

			REQUIRE_THAT(request.get_value(0), value_matcher(pull::PULL_UP));
		}

		REQUIRE_THAT(request.get_value(0), value_matcher(pull::PULL_UP));
	}
}

TEST_CASE("line_request stream insertion operator works", "[line-request]")
{
	auto sim = make_sim()
		.set_num_lines(4)
		.build();

	auto request = ::gpiod::chip(sim.dev_path())
		.prepare_request()
		.add_line_settings({ 3, 1, 0, 2}, ::gpiod::line_settings())
		.do_request();

	::std::stringstream buf, expected;

	expected << "gpiod::line_request(num_lines=4, line_offsets=gpiod::offsets(3, 1, 0, 2), fd=" <<
		    request.fd() << ")";

	SECTION("active request")
	{
		buf << request;

		REQUIRE_THAT(buf.str(), Catch::Equals(expected.str()));
	}

	SECTION("request released")
	{
		request.release();

		buf << request;

		REQUIRE_THAT(buf.str(), Catch::Equals("gpiod::line_request(released)"));
	}
}

} /* namespace */
