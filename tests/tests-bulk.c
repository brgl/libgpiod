// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <errno.h>

#include "gpiod-test.h"

#define GPIOD_TEST_GROUP "bulk"

GPIOD_TEST_CASE(alloc_zero_lines, 0, { 1 })
{
	struct gpiod_line_bulk *bulk;

	bulk = gpiod_line_bulk_new(0);
	g_assert_null(bulk);
	g_assert_cmpint(errno, ==, EINVAL);
}

GPIOD_TEST_CASE(add_too_many_lines, 0, { 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *lineA, *lineB, *lineC;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	bulk = gpiod_line_bulk_new(2);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	lineA = gpiod_chip_get_line(chip, 0);
	lineB = gpiod_chip_get_line(chip, 1);
	lineC = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(lineA);
	g_assert_nonnull(lineB);
	g_assert_nonnull(lineC);
	gpiod_test_return_if_failed();

	ret = gpiod_line_bulk_add_line(bulk, lineA);
	g_assert_cmpint(ret, ==, 0);
	ret = gpiod_line_bulk_add_line(bulk, lineB);
	g_assert_cmpint(ret, ==, 0);
	ret = gpiod_line_bulk_add_line(bulk, lineC);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);
}

GPIOD_TEST_CASE(add_lines_from_different_chips, 0, { 8, 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chipA = NULL;
	g_autoptr(gpiod_chip_struct) chipB = NULL;
	struct gpiod_line *lineA, *lineB;
	gint ret;

	chipA = gpiod_chip_open(gpiod_test_chip_path(0));
	chipB = gpiod_chip_open(gpiod_test_chip_path(1));
	g_assert_nonnull(chipA);
	g_assert_nonnull(chipB);
	gpiod_test_return_if_failed();

	bulk = gpiod_line_bulk_new(4);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	lineA = gpiod_chip_get_line(chipA, 0);
	lineB = gpiod_chip_get_line(chipB, 0);
	g_assert_nonnull(lineA);
	g_assert_nonnull(lineB);
	gpiod_test_return_if_failed();

	ret = gpiod_line_bulk_add_line(bulk, lineA);
	g_assert_cmpint(ret, ==, 0);
	ret = gpiod_line_bulk_add_line(bulk, lineB);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);
}

static const gchar *const bulk_foreach_line_names[] = {
	"gpio-mockup-A-0",
	"gpio-mockup-A-1",
	"gpio-mockup-A-2",
	"gpio-mockup-A-3"
};

static int bulk_foreach_callback_all(struct gpiod_line *line, void *data)
{
	gint *i = data;

	g_assert_cmpstr(gpiod_line_name(line), ==,
			bulk_foreach_line_names[(*i)++]);

	return GPIOD_LINE_BULK_CB_NEXT;
}

static int bulk_foreach_callback_stop(struct gpiod_line *line, void *data)
{
	gint *i = data;

	g_assert_cmpstr(gpiod_line_name(line), ==,
			bulk_foreach_line_names[(*i)++]);
	if (*i == 2)
		return GPIOD_LINE_BULK_CB_STOP;

	return GPIOD_LINE_BULK_CB_NEXT;
}

GPIOD_TEST_CASE(foreach_all_lines, GPIOD_TEST_FLAG_NAMED_LINES, { 4 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	gint i = 0;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	bulk = gpiod_chip_get_all_lines(chip);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_foreach_line(bulk, bulk_foreach_callback_all, &i);
	gpiod_test_return_if_failed();
}

GPIOD_TEST_CASE(foreach_two_lines, GPIOD_TEST_FLAG_NAMED_LINES, { 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	gint i = 0;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	bulk = gpiod_chip_get_all_lines(chip);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_foreach_line(bulk, bulk_foreach_callback_stop, &i);
	g_assert_cmpint(i, ==, 2);
}
