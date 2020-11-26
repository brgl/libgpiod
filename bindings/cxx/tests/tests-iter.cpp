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

TEST_CASE("Line iterator works", "[iter][line]")
{
	mockup::probe_guard mockup_chips({ 4 });
	::gpiod::chip chip(mockup::instance().chip_name(0));
	unsigned int count = 0;

	for (auto& it: ::gpiod::line_iter(chip))
		REQUIRE(it.offset() == count++);

	REQUIRE(count == chip.num_lines());
}
