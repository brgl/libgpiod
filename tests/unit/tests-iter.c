/*
 * Iterator test cases for libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "gpiod-unit.h"

static void chip_iter(void)
{
	GU_CLEANUP(gu_free_chip_iter) struct gpiod_chip_iter *iter = NULL;
	struct gpiod_chip *chip;
	bool A, B, C;

	A = B = C = false;

	iter = gpiod_chip_iter_new();
	GU_ASSERT_NOT_NULL(iter);

	gpiod_foreach_chip(iter, chip) {
		GU_ASSERT(!gpiod_chip_iter_err(iter));

		if (strcmp(gpiod_chip_label(chip), "gpio-mockup-A") == 0)
			A = true;
		else if (strcmp(gpiod_chip_label(chip), "gpio-mockup-B") == 0)
			B = true;
		else if (strcmp(gpiod_chip_label(chip), "gpio-mockup-C") == 0)
			C = true;
	}

	GU_ASSERT(A);
	GU_ASSERT(B);
	GU_ASSERT(C);
}
GU_DEFINE_TEST(chip_iter, "gpiod_chip iterator",
	       GU_LINES_UNNAMED, { 8, 8, 8 });
