// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <catch2/catch.hpp>
#include <gpiod.hpp>

#include "gpio-mockup.hpp"

using ::gpiod::test::mockup;

TEST_CASE("GPIO chip device can be verified with is_gpiochip_device()", "[chip]")
{
	mockup::probe_guard mockup_chips({ 8 });

	SECTION("good chip")
	{
		REQUIRE(::gpiod::is_gpiochip_device(mockup::instance().chip_path(0)));
	}

	SECTION("not a chip")
	{
		REQUIRE_FALSE(::gpiod::is_gpiochip_device("/dev/null"));
	}

	SECTION("nonexistent file")
	{
		REQUIRE_FALSE(::gpiod::is_gpiochip_device("/dev/nonexistent_device"));
	}
}

TEST_CASE("GPIO chip device can be opened", "[chip]")
{
	mockup::probe_guard mockup_chips({ 8, 8, 8 });

	SECTION("open from constructor")
	{
		REQUIRE_NOTHROW(::gpiod::chip(mockup::instance().chip_path(1)));
	}

	SECTION("open from open() method")
	{
		::gpiod::chip chip;

		REQUIRE_FALSE(!!chip);

		REQUIRE_NOTHROW(chip.open(mockup::instance().chip_path(1)));
	}
}

TEST_CASE("Uninitialized GPIO chip behaves correctly", "[chip]")
{
	::gpiod::chip chip;

	SECTION("uninitialized chip is 'false'")
	{
		REQUIRE_FALSE(!!chip);
	}

	SECTION("using uninitialized chip throws logic_error")
	{
		REQUIRE_THROWS_AS(chip.name(), ::std::logic_error);
	}
}

TEST_CASE("Trying to open a nonexistent chip throws system_error", "[chip]")
{
	REQUIRE_THROWS_AS(::gpiod::chip("nonexistent-chip"), ::std::system_error);
}

TEST_CASE("Chip object can be reset", "[chip]")
{
	mockup::probe_guard mockup_chips({ 8 });

	::gpiod::chip chip(mockup::instance().chip_path(0));
	REQUIRE(chip);
	chip.reset();
	REQUIRE_FALSE(!!chip);
}

TEST_CASE("Chip info can be correctly retrieved", "[chip]")
{
	mockup::probe_guard mockup_chips({ 8, 16, 8 });

	::gpiod::chip chip(mockup::instance().chip_path(1));
	REQUIRE(chip.name() == mockup::instance().chip_name(1));
	REQUIRE(chip.label() == "gpio-mockup-B");
	REQUIRE(chip.num_lines() == 16);
}

TEST_CASE("Chip object can be copied and compared", "[chip]")
{
	mockup::probe_guard mockup_chips({ 8, 8 });

	::gpiod::chip chip1(mockup::instance().chip_path(0));
	auto chip2 = chip1;
	REQUIRE(chip1 == chip2);
	REQUIRE_FALSE(chip1 != chip2);
	::gpiod::chip chip3(mockup::instance().chip_path(1));
	REQUIRE(chip1 != chip3);
	REQUIRE_FALSE(chip2 == chip3);
}

TEST_CASE("Lines can be retrieved from chip objects", "[chip]")
{
	mockup::probe_guard mockup_chips({ 8, 8, 8 }, mockup::FLAG_NAMED_LINES);
	::gpiod::chip chip(mockup::instance().chip_path(1));

	SECTION("get single line by offset")
	{
		auto line = chip.get_line(3);
		REQUIRE(line.name() == "gpio-mockup-B-3");
	}

	SECTION("find single line by name")
	{
		auto offset = chip.find_line("gpio-mockup-B-3");
		REQUIRE(offset == 3);
	}

	SECTION("get multiple lines by offsets")
	{
		auto lines = chip.get_lines({ 1, 3, 4, 7});
		REQUIRE(lines.size() == 4);
		REQUIRE(lines.get(0).name() == "gpio-mockup-B-1");
		REQUIRE(lines.get(1).name() == "gpio-mockup-B-3");
		REQUIRE(lines.get(2).name() == "gpio-mockup-B-4");
		REQUIRE(lines.get(3).name() == "gpio-mockup-B-7");
	}

	SECTION("get multiple lines by offsets in mixed order")
	{
		auto lines = chip.get_lines({ 5, 1, 3, 2});
		REQUIRE(lines.size() == 4);
		REQUIRE(lines.get(0).name() == "gpio-mockup-B-5");
		REQUIRE(lines.get(1).name() == "gpio-mockup-B-1");
		REQUIRE(lines.get(2).name() == "gpio-mockup-B-3");
		REQUIRE(lines.get(3).name() == "gpio-mockup-B-2");
	}
}

TEST_CASE("All lines can be retrieved from a chip at once", "[chip]")
{
	mockup::probe_guard mockup_chips({ 4 });
	::gpiod::chip chip(mockup::instance().chip_path(0));

	auto lines = chip.get_all_lines();
	REQUIRE(lines.size() == 4);
	REQUIRE(lines.get(0).offset() == 0);
	REQUIRE(lines.get(1).offset() == 1);
	REQUIRE(lines.get(2).offset() == 2);
	REQUIRE(lines.get(3).offset() == 3);
}

TEST_CASE("Errors occurring when retrieving lines are correctly reported", "[chip]")
{
	mockup::probe_guard mockup_chips({ 8 }, mockup::FLAG_NAMED_LINES);
	::gpiod::chip chip(mockup::instance().chip_path(0));

	SECTION("invalid offset (single line)")
	{
		REQUIRE_THROWS_AS(chip.get_line(9), ::std::out_of_range);
	}

	SECTION("invalid offset (multiple lines)")
	{
		REQUIRE_THROWS_AS(chip.get_lines({ 1, 19, 4, 7 }), ::std::out_of_range);
	}

	SECTION("line not found by name")
	{
		REQUIRE(chip.find_line("nonexistent-line") == -1);
	}
}
