// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <gpiod-glib.h>
#include <gpiod-test.h>
#include <gpiod-test-common.h>
#include <gpiosim-glib.h>

#include "helpers.h"

#define GPIOD_TEST_GROUP "glib/chip-info"

GPIOD_TEST_CASE(get_name)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GpiodglibChipInfo) info = NULL;
	g_autofree gchar *name = NULL;

	chip = gpiodglib_test_new_chip_or_fail(
			g_gpiosim_chip_get_dev_path(sim));

	info = gpiodglib_test_chip_get_info_or_fail(chip);
	name = gpiodglib_chip_info_dup_name(info);

	g_assert_cmpstr(name, ==, g_gpiosim_chip_get_name(sim));
}

GPIOD_TEST_CASE(get_label)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("label", "foobar",
							NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GpiodglibChipInfo) info = NULL;
	g_autofree gchar *label = NULL;

	chip = gpiodglib_test_new_chip_or_fail(
			g_gpiosim_chip_get_dev_path(sim));

	info = gpiodglib_test_chip_get_info_or_fail(chip);
	label = gpiodglib_chip_info_dup_label(info);

	g_assert_cmpstr(label, ==, "foobar");
}

GPIOD_TEST_CASE(get_num_lines)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 16, NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GpiodglibChipInfo) info = NULL;

	chip = gpiodglib_test_new_chip_or_fail(
			g_gpiosim_chip_get_dev_path(sim));

	info = gpiodglib_test_chip_get_info_or_fail(chip);

	g_assert_cmpuint(gpiodglib_chip_info_get_num_lines(info), ==, 16);
}
