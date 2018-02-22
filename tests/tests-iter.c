/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 */

/* Iterator test cases. */

#include "gpiod-test.h"

static void chip_iter(void)
{
	TEST_CLEANUP(test_free_chip_iter) struct gpiod_chip_iter *iter = NULL;
	struct gpiod_chip *chip;
	bool A, B, C;

	A = B = C = false;

	iter = gpiod_chip_iter_new();
	TEST_ASSERT_NOT_NULL(iter);

	gpiod_foreach_chip(iter, chip) {
		if (strcmp(gpiod_chip_label(chip), "gpio-mockup-A") == 0)
			A = true;
		else if (strcmp(gpiod_chip_label(chip), "gpio-mockup-B") == 0)
			B = true;
		else if (strcmp(gpiod_chip_label(chip), "gpio-mockup-C") == 0)
			C = true;
	}

	TEST_ASSERT(A);
	TEST_ASSERT(B);
	TEST_ASSERT(C);
}
TEST_DEFINE(chip_iter,
	    "gpiod_chip_iter - simple loop",
	    0, { 8, 8, 8 });

static void chip_iter_noclose(void)
{
	TEST_CLEANUP(test_free_chip_iter_noclose)
			struct gpiod_chip_iter *iter = NULL;
	TEST_CLEANUP_CHIP struct gpiod_chip *chipA = NULL;
	TEST_CLEANUP_CHIP struct gpiod_chip *chipB = NULL;
	TEST_CLEANUP_CHIP struct gpiod_chip *chipC = NULL;
	struct gpiod_chip *chip;
	bool A, B, C;

	A = B = C = false;

	iter = gpiod_chip_iter_new();
	TEST_ASSERT_NOT_NULL(iter);

	gpiod_foreach_chip_noclose(iter, chip) {
		if (strcmp(gpiod_chip_label(chip), "gpio-mockup-A") == 0) {
			A = true;
			chipA = chip;
		} else if (strcmp(gpiod_chip_label(chip),
				  "gpio-mockup-B") == 0) {
			B = true;
			chipB = chip;
		} else if (strcmp(gpiod_chip_label(chip),
				  "gpio-mockup-C") == 0) {
			C = true;
			chipC = chip;
		}
	}

	TEST_ASSERT(A);
	TEST_ASSERT(B);
	TEST_ASSERT(C);

	gpiod_chip_iter_free_noclose(iter);
	iter = NULL;

	/* See if the chips are still open and usable. */
	TEST_ASSERT_STR_EQ(gpiod_chip_label(chipA), "gpio-mockup-A");
	TEST_ASSERT_STR_EQ(gpiod_chip_label(chipB), "gpio-mockup-B");
	TEST_ASSERT_STR_EQ(gpiod_chip_label(chipC), "gpio-mockup-C");
}
TEST_DEFINE(chip_iter_noclose,
	    "gpiod_chip_iter - simple loop, noclose variant",
	    0, { 8, 8, 8 });

static void chip_iter_break(void)
{
	TEST_CLEANUP(test_free_chip_iter) struct gpiod_chip_iter *iter = NULL;
	struct gpiod_chip *chip;
	int i = 0;

	iter = gpiod_chip_iter_new();
	TEST_ASSERT_NOT_NULL(iter);

	gpiod_foreach_chip(iter, chip) {
		if ((strcmp(gpiod_chip_label(chip), "gpio-mockup-A") == 0) ||
		    (strcmp(gpiod_chip_label(chip), "gpio-mockup-B") == 0) ||
		    (strcmp(gpiod_chip_label(chip), "gpio-mockup-C") == 0))
			i++;

		if (i == 3)
			break;
	}

	gpiod_chip_iter_free(iter);
	iter = NULL;

	TEST_ASSERT_EQ(i, 3);
}
TEST_DEFINE(chip_iter_break,
	    "gpiod_chip_iter - break",
	    0, { 8, 8, 8, 8, 8 });

static void line_iter(void)
{
	TEST_CLEANUP(test_free_line_iter) struct gpiod_line_iter *iter = NULL;
	TEST_CLEANUP_CHIP struct gpiod_chip *chip = NULL;
	struct gpiod_line *line;
	unsigned int i = 0;

	chip = gpiod_chip_open(test_chip_path(0));
	TEST_ASSERT_NOT_NULL(chip);

	iter = gpiod_line_iter_new(chip);
	TEST_ASSERT_NOT_NULL(iter);

	gpiod_foreach_line(iter, line) {
		TEST_ASSERT_EQ(i, gpiod_line_offset(line));
		i++;
	}

	TEST_ASSERT_EQ(8, i);
}
TEST_DEFINE(line_iter,
	    "gpiod_line_iter - simple loop, check offsets",
	    0, { 8 });
