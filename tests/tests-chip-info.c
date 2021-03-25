// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2022 Bartosz Golaszewski <bartekgola@gmail.com>

#include <errno.h>
#include <glib.h>
#include <gpiod.h>

#include "gpiod-test.h"
#include "gpiod-test-helpers.h"
#include "gpiod-test-sim.h"

#define GPIOD_TEST_GROUP "chip-info"

GPIOD_TEST_CASE(get_chip_info_name)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new(NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_chip_info) info = NULL;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	info = gpiod_test_get_chip_info_or_fail(chip);

	g_assert_cmpstr(gpiod_chip_info_get_name(info), ==,
			g_gpiosim_chip_get_name(sim));
}

GPIOD_TEST_CASE(get_chip_info_label)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("label", "foobar",
							NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_chip_info) info = NULL;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	info = gpiod_test_get_chip_info_or_fail(chip);

	g_assert_cmpstr(gpiod_chip_info_get_label(info), ==, "foobar");
}

GPIOD_TEST_CASE(get_num_lines)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 16, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_chip_info) info = NULL;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	info = gpiod_test_get_chip_info_or_fail(chip);

	g_assert_cmpuint(gpiod_chip_info_get_num_lines(info), ==, 16);
}

