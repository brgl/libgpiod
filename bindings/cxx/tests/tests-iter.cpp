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

TEST_CASE("Chip iterator works", "[iter][chip]")
{
	mockup::probe_guard mockup_chips({ 8, 8, 8 });
	bool gotA = false, gotB = false, gotC = false;

	for (auto& it: ::gpiod::make_chip_iter()) {
		if (it.label() == "gpio-mockup-A")
			gotA = true;
		if (it.label() == "gpio-mockup-B")
			gotB = true;
		if (it.label() == "gpio-mockup-C")
			gotC = true;
	}

	REQUIRE(gotA);
	REQUIRE(gotB);
	REQUIRE(gotC);
}

TEST_CASE("Chip iterator loop can be broken out of", "[iter][chip]")
{
	mockup::probe_guard mockup_chips({ 8, 8, 8, 8, 8, 8 });
	int count_chips = 0;

	for (auto& it: ::gpiod::make_chip_iter()) {
		if (it.label() == "gpio-mockup-A" ||
		    it.label() == "gpio-mockup-B" ||
		    it.label() == "gpio-mockup-C")
			count_chips++;

		if (count_chips == 3)
			break;
	}

	REQUIRE(count_chips == 3);
}

TEST_CASE("Line iterator works", "[iter][line]")
{
	mockup::probe_guard mockup_chips({ 4 });
	::gpiod::chip chip(mockup::instance().chip_name(0));
	int count = 0;

	for (auto& it: ::gpiod::line_iter(chip))
		REQUIRE(it.offset() == count++);

	REQUIRE(count == chip.num_lines());
}
