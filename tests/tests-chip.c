// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <errno.h>
#include <glib.h>
#include <gpiod.h>

#include "gpiod-test.h"
#include "gpiod-test-helpers.h"
#include "gpiod-test-sim.h"

#define GPIOD_TEST_GROUP "chip"

GPIOD_TEST_CASE(open_chip_good)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;

	chip = gpiod_chip_open(g_gpiosim_chip_get_dev_path(sim));
	g_assert_nonnull(chip);
}

GPIOD_TEST_CASE(open_chip_nonexistent)
{
	g_autoptr(struct_gpiod_chip) chip = NULL;

	chip = gpiod_chip_open("/dev/nonexistent");
	g_assert_null(chip);
	gpiod_test_expect_errno(ENOENT);
}

GPIOD_TEST_CASE(open_chip_not_a_character_device)
{
	g_autoptr(struct_gpiod_chip) chip = NULL;

	chip = gpiod_chip_open("/tmp");
	g_assert_null(chip);
	gpiod_test_expect_errno(ENOTTY);
}

GPIOD_TEST_CASE(open_chip_not_a_gpio_device)
{
	g_autoptr(struct_gpiod_chip) chip = NULL;

	chip = gpiod_chip_open("/dev/null");
	g_assert_null(chip);
	gpiod_test_expect_errno(ENODEV);
}

GPIOD_TEST_CASE(open_chip_null_path)
{
	g_autoptr(struct_gpiod_chip) chip = NULL;

	chip = gpiod_chip_open(NULL);
	g_assert_null(chip);
	gpiod_test_expect_errno(EINVAL);
}

GPIOD_TEST_CASE(get_chip_path)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	const gchar *path = g_gpiosim_chip_get_dev_path(sim);

	chip = gpiod_test_open_chip_or_fail(path);

	g_assert_cmpstr(gpiod_chip_get_path(chip), ==, path);
}

GPIOD_TEST_CASE(get_fd)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));

	g_assert_cmpint(gpiod_chip_get_fd(chip), >=, 0);
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

	g_autoptr(GPIOSimChip) sim = NULL;
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(GVariant) vnames = gpiod_test_package_line_names(names);

	sim = g_gpiosim_chip_new(
			"num-lines", 8,
			"line-names", vnames,
			 NULL);

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));

	g_assert_cmpint(
		gpiod_chip_get_line_offset_from_name(chip,
						     "nonexistent"), ==, -1);
	gpiod_test_expect_errno(ENOENT);
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

	g_autoptr(GPIOSimChip) sim = NULL;
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(GVariant) vnames = gpiod_test_package_line_names(names);

	sim = g_gpiosim_chip_new(
			"num-lines", 8,
			"line-names", vnames,
			NULL);

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));

	g_assert_cmpint(gpiod_chip_get_line_offset_from_name(chip, "baz"),
			==, 4);
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

	g_autoptr(GPIOSimChip) sim = NULL;
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(GVariant) vnames = gpiod_test_package_line_names(names);

	sim = g_gpiosim_chip_new(
			"num-lines", 8,
			"line-names", vnames,
			NULL);

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));

	g_assert_cmpint(gpiod_chip_get_line_offset_from_name(chip, "baz"),
			==, 2);
}

GPIOD_TEST_CASE(find_line_null_name)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	gint ret;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));

	ret = gpiod_chip_get_line_offset_from_name(chip, NULL);
	g_assert_cmpint(ret, ==, -1);
	gpiod_test_expect_errno(EINVAL);
}
