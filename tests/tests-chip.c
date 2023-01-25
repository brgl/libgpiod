// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>

#include "gpiod-test.h"

#define GPIOD_TEST_GROUP "chip"

GPIOD_TEST_CASE(open_good, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
}

GPIOD_TEST_CASE(open_nonexistent, 0, { 8 })
{
	struct gpiod_chip *chip;

	chip = gpiod_chip_open("/dev/nonexistent_gpiochip");
	g_assert_null(chip);
	g_assert_cmpint(errno, ==, ENOENT);
}

GPIOD_TEST_CASE(open_notty, 0, { 8 })
{
	struct gpiod_chip *chip;

	chip = gpiod_chip_open("/dev/null");
	g_assert_null(chip);
	g_assert_cmpint(errno, ==, ENOTTY);
}

GPIOD_TEST_CASE(open_by_name_good, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;

	chip = gpiod_chip_open_by_name(gpiod_test_chip_name(0));
	g_assert_nonnull(chip);
}

GPIOD_TEST_CASE(open_by_number_good, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;

	chip = gpiod_chip_open_by_number(gpiod_test_chip_num(0));
	g_assert_nonnull(chip);
}

GPIOD_TEST_CASE(open_by_label_good, 0, { 4, 4, 4, 4, 4 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;

	chip = gpiod_chip_open_by_label("gpio-mockup-D");
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();
	g_assert_cmpstr(gpiod_chip_name(chip), ==, gpiod_test_chip_name(3));
}

GPIOD_TEST_CASE(open_by_label_bad, 0, { 4, 4, 4, 4, 4 })
{
	struct gpiod_chip *chip;

	chip = gpiod_chip_open_by_label("nonexistent_gpio_chip");
	g_assert_null(chip);
	g_assert_cmpint(errno, ==, ENOENT);
}

GPIOD_TEST_CASE(open_lookup_good, 0, { 8, 8, 8})
{
	g_autoptr(gpiod_chip_struct) chip_by_label = NULL;
	g_autoptr(gpiod_chip_struct) chip_by_name = NULL;
	g_autoptr(gpiod_chip_struct) chip_by_path = NULL;
	g_autoptr(gpiod_chip_struct) chip_by_num = NULL;
	g_autofree gchar *chip_num_str = g_strdup_printf(
						"%u", gpiod_test_chip_num(1));

	chip_by_name = gpiod_chip_open_lookup(gpiod_test_chip_name(1));
	chip_by_path = gpiod_chip_open_lookup(gpiod_test_chip_path(1));
	chip_by_num = gpiod_chip_open_lookup(chip_num_str);
	chip_by_label = gpiod_chip_open_lookup("gpio-mockup-B");

	g_assert_nonnull(chip_by_name);
	g_assert_nonnull(chip_by_path);
	g_assert_nonnull(chip_by_num);
	g_assert_nonnull(chip_by_label);
	gpiod_test_return_if_failed();

	g_assert_cmpstr(gpiod_chip_name(chip_by_name),
			==, gpiod_test_chip_name(1));
	g_assert_cmpstr(gpiod_chip_name(chip_by_path),
			==, gpiod_test_chip_name(1));
	g_assert_cmpstr(gpiod_chip_name(chip_by_num),
			==, gpiod_test_chip_name(1));
	g_assert_cmpstr(gpiod_chip_name(chip_by_label),
			==, gpiod_test_chip_name(1));
}

GPIOD_TEST_CASE(get_name, 0, { 8, 8, 8})
{
	g_autoptr(gpiod_chip_struct) chip0 = NULL;
	g_autoptr(gpiod_chip_struct) chip1 = NULL;
	g_autoptr(gpiod_chip_struct) chip2 = NULL;

	chip0 = gpiod_chip_open(gpiod_test_chip_path(0));
	chip1 = gpiod_chip_open(gpiod_test_chip_path(1));
	chip2 = gpiod_chip_open(gpiod_test_chip_path(2));

	g_assert_nonnull(chip0);
	g_assert_nonnull(chip1);
	g_assert_nonnull(chip2);
	gpiod_test_return_if_failed();

	g_assert_cmpstr(gpiod_chip_name(chip0), ==, gpiod_test_chip_name(0));
	g_assert_cmpstr(gpiod_chip_name(chip1), ==, gpiod_test_chip_name(1));
	g_assert_cmpstr(gpiod_chip_name(chip2), ==, gpiod_test_chip_name(2));
}

GPIOD_TEST_CASE(get_label, 0, { 8, 8, 8})
{
	g_autoptr(gpiod_chip_struct) chip0 = NULL;
	g_autoptr(gpiod_chip_struct) chip1 = NULL;
	g_autoptr(gpiod_chip_struct) chip2 = NULL;

	chip0 = gpiod_chip_open(gpiod_test_chip_path(0));
	chip1 = gpiod_chip_open(gpiod_test_chip_path(1));
	chip2 = gpiod_chip_open(gpiod_test_chip_path(2));

	g_assert_nonnull(chip0);
	g_assert_nonnull(chip1);
	g_assert_nonnull(chip2);
	gpiod_test_return_if_failed();

	g_assert_cmpstr(gpiod_chip_label(chip0), ==, "gpio-mockup-A");
	g_assert_cmpstr(gpiod_chip_label(chip1), ==, "gpio-mockup-B");
	g_assert_cmpstr(gpiod_chip_label(chip2), ==, "gpio-mockup-C");
}

GPIOD_TEST_CASE(num_lines, 0, { 1, 4, 8, 16, 32 })
{
	g_autoptr(gpiod_chip_struct) chip0 = NULL;
	g_autoptr(gpiod_chip_struct) chip1 = NULL;
	g_autoptr(gpiod_chip_struct) chip2 = NULL;
	g_autoptr(gpiod_chip_struct) chip3 = NULL;
	g_autoptr(gpiod_chip_struct) chip4 = NULL;

	chip0 = gpiod_chip_open(gpiod_test_chip_path(0));
	chip1 = gpiod_chip_open(gpiod_test_chip_path(1));
	chip2 = gpiod_chip_open(gpiod_test_chip_path(2));
	chip3 = gpiod_chip_open(gpiod_test_chip_path(3));
	chip4 = gpiod_chip_open(gpiod_test_chip_path(4));

	g_assert_nonnull(chip0);
	g_assert_nonnull(chip1);
	g_assert_nonnull(chip2);
	g_assert_nonnull(chip3);
	g_assert_nonnull(chip4);
	gpiod_test_return_if_failed();

	g_assert_cmpuint(gpiod_chip_num_lines(chip0), ==, 1);
	g_assert_cmpuint(gpiod_chip_num_lines(chip1), ==, 4);
	g_assert_cmpuint(gpiod_chip_num_lines(chip2), ==, 8);
	g_assert_cmpuint(gpiod_chip_num_lines(chip3), ==, 16);
	g_assert_cmpuint(gpiod_chip_num_lines(chip4), ==, 32);
}

GPIOD_TEST_CASE(get_line, 0, { 16 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 3);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();
	g_assert_cmpuint(gpiod_line_offset(line), ==, 3);
}

GPIOD_TEST_CASE(get_lines, 0, { 16 })
{
	struct gpiod_line *line0, *line1, *line2, *line3;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_bulk bulk;
	guint offsets[4];
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	offsets[0] = 1;
	offsets[1] = 3;
	offsets[2] = 4;
	offsets[3] = 7;

	ret = gpiod_chip_get_lines(chip, offsets, 4, &bulk);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpuint(gpiod_line_bulk_num_lines(&bulk), ==, 4);
	gpiod_test_return_if_failed();

	line0 = gpiod_line_bulk_get_line(&bulk, 0);
	line1 = gpiod_line_bulk_get_line(&bulk, 1);
	line2 = gpiod_line_bulk_get_line(&bulk, 2);
	line3 = gpiod_line_bulk_get_line(&bulk, 3);

	g_assert_cmpuint(gpiod_line_offset(line0), ==, 1);
	g_assert_cmpuint(gpiod_line_offset(line1), ==, 3);
	g_assert_cmpuint(gpiod_line_offset(line2), ==, 4);
	g_assert_cmpuint(gpiod_line_offset(line3), ==, 7);
}

GPIOD_TEST_CASE(get_all_lines, 0, { 4 })
{
	struct gpiod_line *line0, *line1, *line2, *line3;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_bulk bulk;
	int ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_get_all_lines(chip, &bulk);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpuint(gpiod_line_bulk_num_lines(&bulk), ==, 4);
	gpiod_test_return_if_failed();

	line0 = gpiod_line_bulk_get_line(&bulk, 0);
	line1 = gpiod_line_bulk_get_line(&bulk, 1);
	line2 = gpiod_line_bulk_get_line(&bulk, 2);
	line3 = gpiod_line_bulk_get_line(&bulk, 3);

	g_assert_cmpuint(gpiod_line_offset(line0), ==, 0);
	g_assert_cmpuint(gpiod_line_offset(line1), ==, 1);
	g_assert_cmpuint(gpiod_line_offset(line2), ==, 2);
	g_assert_cmpuint(gpiod_line_offset(line3), ==, 3);
}

GPIOD_TEST_CASE(find_line_good, GPIOD_TEST_FLAG_NAMED_LINES, { 8, 8, 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;

	chip = gpiod_chip_open(gpiod_test_chip_path(1));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_find_line(chip, "gpio-mockup-B-4");
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();
	g_assert_cmpuint(gpiod_line_offset(line), ==, 4);
	g_assert_cmpstr(gpiod_line_name(line), ==, "gpio-mockup-B-4");
}

GPIOD_TEST_CASE(find_line_not_found, GPIOD_TEST_FLAG_NAMED_LINES, { 8, 8, 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;

	chip = gpiod_chip_open(gpiod_test_chip_path(1));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_find_line(chip, "nonexistent");
	g_assert_null(line);
	g_assert_cmpint(errno, ==, ENOENT);
}

GPIOD_TEST_CASE(find_lines_good, GPIOD_TEST_FLAG_NAMED_LINES, { 8, 8, 8 })
{
	static const gchar *names[] = { "gpio-mockup-B-3",
					"gpio-mockup-B-6",
					"gpio-mockup-B-7",
					NULL };

	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line0, *line1, *line2;
	struct gpiod_line_bulk bulk;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(1));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_find_lines(chip, names, &bulk);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpuint(gpiod_line_bulk_num_lines(&bulk), ==, 3);
	gpiod_test_return_if_failed();

	line0 = gpiod_line_bulk_get_line(&bulk, 0);
	line1 = gpiod_line_bulk_get_line(&bulk, 1);
	line2 = gpiod_line_bulk_get_line(&bulk, 2);

	g_assert_cmpuint(gpiod_line_offset(line0), ==, 3);
	g_assert_cmpuint(gpiod_line_offset(line1), ==, 6);
	g_assert_cmpuint(gpiod_line_offset(line2), ==, 7);
}

GPIOD_TEST_CASE(fine_lines_not_found, GPIOD_TEST_FLAG_NAMED_LINES, { 8, 8, 8 })
{
	static const gchar *names[] = { "gpio-mockup-B-3",
					"nonexistent",
					"gpio-mockup-B-7",
					NULL };

	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_bulk bulk;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(1));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	ret = gpiod_chip_find_lines(chip, names, &bulk);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, ENOENT);
}
