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
GU_DEFINE_TEST(chip_iter,
	       "gpiod_chip_iter - simple loop",
	       GU_LINES_UNNAMED, { 8, 8, 8 });

static void chip_iter_noclose(void)
{
	GU_CLEANUP(gu_free_chip_iter_noclose)
			struct gpiod_chip_iter *iter = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chipA;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chipB;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chipC;
	struct gpiod_chip *chip;
	bool A, B, C;

	A = B = C = false;

	iter = gpiod_chip_iter_new();
	GU_ASSERT_NOT_NULL(iter);

	gpiod_foreach_chip_noclose(iter, chip) {
		GU_ASSERT(!gpiod_chip_iter_err(iter));

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

	GU_ASSERT(A);
	GU_ASSERT(B);
	GU_ASSERT(C);

	gpiod_chip_iter_free_noclose(iter);
	iter = NULL;

	/* See if the chips are still open and usable. */
	GU_ASSERT_STR_EQ(gpiod_chip_label(chipA), "gpio-mockup-A");
	GU_ASSERT_STR_EQ(gpiod_chip_label(chipB), "gpio-mockup-B");
	GU_ASSERT_STR_EQ(gpiod_chip_label(chipC), "gpio-mockup-C");
}
GU_DEFINE_TEST(chip_iter_noclose,
	       "gpiod_chip_iter - simple loop, noclose variant",
	       GU_LINES_UNNAMED, { 8, 8, 8 });

static void line_iter(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;
	struct gpiod_line_iter iter;
	struct gpiod_line *line;
	unsigned int i = 0;

	chip = gpiod_chip_open(gu_chip_path(0));
	GU_ASSERT_NOT_NULL(chip);

	gpiod_line_iter_init(&iter, chip);

	gpiod_foreach_line(&iter, line) {
		GU_ASSERT(!gpiod_line_iter_err(&iter));
		GU_ASSERT_EQ(i, gpiod_line_offset(line));
		i++;
	}

	GU_ASSERT_EQ(8, i);
}
GU_DEFINE_TEST(line_iter,
	       "gpiod_line_iter - simple loop, check offsets",
	       GU_LINES_UNNAMED, { 8 });

static void line_iter_static_initializer(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;
	struct gpiod_line *line;
	unsigned int i = 0;

	chip = gpiod_chip_open(gu_chip_path(0));
	GU_ASSERT_NOT_NULL(chip);

	{
		struct gpiod_line_iter iter = GPIOD_LINE_ITER_INITIALIZER(chip);

		gpiod_foreach_line(&iter, line) {
			GU_ASSERT(!gpiod_line_iter_err(&iter));
			GU_ASSERT_EQ(i, gpiod_line_offset(line));
			i++;
		}
	}

	GU_ASSERT_EQ(8, i);
}
GU_DEFINE_TEST(line_iter_static_initializer,
	       "gpiod_line_iter - simple loop, static initializer",
	       GU_LINES_UNNAMED, { 8 });
