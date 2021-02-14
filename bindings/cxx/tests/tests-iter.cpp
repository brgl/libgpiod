// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <catch2/catch.hpp>
#include <gpiod.hpp>

#include "gpio-mockup.hpp"

using ::gpiod::test::mockup;

TEST_CASE("Line iterator works", "[iter][line]")
{
	mockup::probe_guard mockup_chips({ 4 });
	::gpiod::chip chip(mockup::instance().chip_path(0));
	unsigned int count = 0;

	for (auto& it: ::gpiod::line_iter(chip))
		REQUIRE(it.offset() == count++);

	REQUIRE(count == chip.num_lines());
}
