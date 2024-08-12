// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <glib.h>
#include <gpiod-glib.h>
#include <gpiod-test.h>
#include <gpiod-test-common.h>
#include <gpiosim-glib.h>

#include "helpers.h"

#define GPIOD_TEST_GROUP "glib/chip"

GPIOD_TEST_CASE(open_chip_good)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GError) err = NULL;

	chip = gpiodglib_chip_new(g_gpiosim_chip_get_dev_path(sim), &err);
	g_assert_nonnull(chip);
	g_assert_null(err);
}

GPIOD_TEST_CASE(open_chip_nonexistent)
{
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GError) err = NULL;

	chip = gpiodglib_chip_new("/dev/nonexistent", &err);
	g_assert_null(chip);
	gpiodglib_test_check_error_or_fail(err, GPIODGLIB_ERROR,
					   GPIODGLIB_ERR_NOENT);
}

GPIOD_TEST_CASE(open_chip_not_a_character_device)
{
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GError) err = NULL;

	chip = gpiodglib_chip_new("/tmp", &err);
	g_assert_null(chip);
	gpiodglib_test_check_error_or_fail(err, GPIODGLIB_ERROR,
					   GPIODGLIB_ERR_NOTTY);
}

GPIOD_TEST_CASE(open_chip_not_a_gpio_device)
{
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GError) err = NULL;

	chip = gpiodglib_chip_new("/dev/null", &err);
	g_assert_null(chip);
	gpiodglib_test_check_error_or_fail(err, GPIODGLIB_ERROR,
					   GPIODGLIB_ERR_NODEV);
}

GPIOD_TEST_CASE(get_chip_path)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	const gchar *path = g_gpiosim_chip_get_dev_path(sim);
	g_autofree gchar *chip_path = NULL;

	chip = gpiodglib_test_new_chip_or_fail(path);

	chip_path = gpiodglib_chip_dup_path(chip);
	g_assert_cmpstr(chip_path, ==, path);
}

GPIOD_TEST_CASE(closed_chip)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GError) err = NULL;
	g_autoptr(GpiodglibChipInfo) info = NULL;
	const gchar *path = g_gpiosim_chip_get_dev_path(sim);
	g_autofree gchar *chip_path = NULL;

	chip = gpiodglib_test_new_chip_or_fail(path);

	gpiodglib_chip_close(chip);

	info = gpiodglib_chip_get_info(chip, &err);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_CHIP_CLOSED);

	/* Properties still work. */
	chip_path = gpiodglib_chip_dup_path(chip);
	g_assert_cmpstr(chip_path, ==, path);
}

GPIOD_TEST_CASE(find_line_bad)
{
	static const GPIOSimLineName names[] = {
		{ .offset = 1, .name = "foo", },
		{ .offset = 2, .name = "bar", },
		{ .offset = 4, .name = "baz", },
		{ .offset = 5, .name = "xyz", },
		{ }
	};

	g_autoptr(GVariant) vnames = g_gpiosim_package_line_names(names);
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8,
							"line-names", vnames,
							NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GError) err = NULL;
	guint offset;

	chip = gpiodglib_test_new_chip_or_fail(
			g_gpiosim_chip_get_dev_path(sim));

	g_assert_false(gpiodglib_chip_get_line_offset_from_name(chip,
								"nonexistent",
								&offset, &err));
	g_assert_no_error(err);
}

GPIOD_TEST_CASE(find_line_good)
{
	static const GPIOSimLineName names[] = {
		{ .offset = 1, .name = "foo", },
		{ .offset = 2, .name = "bar", },
		{ .offset = 4, .name = "baz", },
		{ .offset = 5, .name = "xyz", },
		{ }
	};

	g_autoptr(GVariant) vnames = g_gpiosim_package_line_names(names);
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8,
							"line-names", vnames,
							NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GError) err = NULL;
	guint offset;

	chip = gpiodglib_test_new_chip_or_fail(
			g_gpiosim_chip_get_dev_path(sim));

	g_assert_true(gpiodglib_chip_get_line_offset_from_name(chip, "baz",
							       &offset, &err));
	g_assert_no_error(err);
	g_assert_cmpuint(offset, ==, 4);
}

/* Verify that for duplicated line names, the first one is returned. */
GPIOD_TEST_CASE(find_line_duplicate)
{
	static const GPIOSimLineName names[] = {
		{ .offset = 1, .name = "foo", },
		{ .offset = 2, .name = "baz", },
		{ .offset = 4, .name = "baz", },
		{ .offset = 5, .name = "xyz", },
		{ }
	};

	g_autoptr(GVariant) vnames = g_gpiosim_package_line_names(names);
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8,
							"line-names", vnames,
							NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GError) err = NULL;
	guint offset;

	chip = gpiodglib_test_new_chip_or_fail(
			g_gpiosim_chip_get_dev_path(sim));

	g_assert_true(gpiodglib_chip_get_line_offset_from_name(chip, "baz",
							       &offset, &err));
	g_assert_no_error(err);
	g_assert_cmpuint(offset, ==, 2);
}

GPIOD_TEST_CASE(find_line_null_name)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GError) err = NULL;
	guint offset;

	chip = gpiodglib_test_new_chip_or_fail(
			g_gpiosim_chip_get_dev_path(sim));

	g_assert_false(gpiodglib_chip_get_line_offset_from_name(chip, NULL,
								&offset, &err));
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL);
}
