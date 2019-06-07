// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <string.h>

#include "gpiod-test.h"

#define GPIOD_TEST_GROUP "iter"

GPIOD_TEST_CASE(chip_iter, 0, { 8, 8, 8 })
{
	g_autoptr(gpiod_chip_iter_struct) iter = NULL;
	struct gpiod_chip *chip;
	gboolean A, B, C;

	A = B = C = FALSE;

	iter = gpiod_chip_iter_new();
	g_assert_nonnull(iter);
	gpiod_test_return_if_failed();

	gpiod_foreach_chip(iter, chip) {
		if (strcmp(gpiod_chip_label(chip), "gpio-mockup-A") == 0)
			A = TRUE;
		else if (strcmp(gpiod_chip_label(chip), "gpio-mockup-B") == 0)
			B = TRUE;
		else if (strcmp(gpiod_chip_label(chip), "gpio-mockup-C") == 0)
			C = TRUE;
	}

	g_assert_true(A);
	g_assert_true(B);
	g_assert_true(C);
}

GPIOD_TEST_CASE(chip_iter_no_close, 0, { 8, 8, 8 })
{
	g_autoptr(gpiod_chip_iter_struct) iter = NULL;
	g_autoptr(gpiod_chip_struct) chipA = NULL;
	g_autoptr(gpiod_chip_struct) chipB = NULL;
	g_autoptr(gpiod_chip_struct) chipC = NULL;
	struct gpiod_chip *chip;

	iter = gpiod_chip_iter_new();
	g_assert_nonnull(iter);
	gpiod_test_return_if_failed();

	gpiod_foreach_chip_noclose(iter, chip) {
		if (strcmp(gpiod_chip_label(chip), "gpio-mockup-A") == 0)
			chipA = chip;
		else if (strcmp(gpiod_chip_label(chip), "gpio-mockup-B") == 0)
			chipB = chip;
		else if (strcmp(gpiod_chip_label(chip), "gpio-mockup-C") == 0)
			chipC = chip;
		else
			gpiod_chip_close(chip);
	}

	g_assert_nonnull(chipA);
	g_assert_nonnull(chipB);
	g_assert_nonnull(chipC);

	gpiod_chip_iter_free_noclose(iter);
	iter = NULL;

	/* See if the chips are still open and usable. */
	g_assert_cmpstr(gpiod_chip_label(chipA), ==, "gpio-mockup-A");
	g_assert_cmpstr(gpiod_chip_label(chipB), ==, "gpio-mockup-B");
	g_assert_cmpstr(gpiod_chip_label(chipC), ==, "gpio-mockup-C");
}

GPIOD_TEST_CASE(chip_iter_break, 0, { 8, 8, 8, 8, 8 })
{
	g_autoptr(gpiod_chip_iter_struct) iter = NULL;
	struct gpiod_chip *chip;
	guint i = 0;

	iter = gpiod_chip_iter_new();
	g_assert_nonnull(iter);
	gpiod_test_return_if_failed();

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

	g_assert_cmpuint(i, ==, 3);
}

GPIOD_TEST_CASE(line_iter, 0, { 8 })
{
	g_autoptr(gpiod_line_iter_struct) iter = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	guint i = 0;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	iter = gpiod_line_iter_new(chip);
	g_assert_nonnull(iter);
	gpiod_test_return_if_failed();

	gpiod_foreach_line(iter, line) {
		g_assert_cmpuint(i, ==, gpiod_line_offset(line));
		i++;
	}

	g_assert_cmpuint(i, ==, 8);
}
