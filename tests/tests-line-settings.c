// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Bartosz Golaszewski <brgl@bgdev.pl>

#include <errno.h>
#include <glib.h>
#include <gpiod.h>

#include "gpiod-test.h"
#include "gpiod-test-helpers.h"

#define GPIOD_TEST_GROUP "line-settings"

GPIOD_TEST_CASE(default_config)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;

	settings = gpiod_test_create_line_settings_or_fail();

	g_assert_cmpint(gpiod_line_settings_get_direction(settings), ==,
			GPIOD_LINE_DIRECTION_AS_IS);
	g_assert_cmpint(gpiod_line_settings_get_edge_detection(settings), ==,
			GPIOD_LINE_EDGE_NONE);
	g_assert_cmpint(gpiod_line_settings_get_bias(settings), ==,
			GPIOD_LINE_BIAS_AS_IS);
	g_assert_cmpint(gpiod_line_settings_get_drive(settings), ==,
			GPIOD_LINE_DRIVE_PUSH_PULL);
	g_assert_false(gpiod_line_settings_get_active_low(settings));
	g_assert_cmpuint(gpiod_line_settings_get_debounce_period_us(settings),
			 ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_event_clock(settings), ==,
			GPIOD_LINE_CLOCK_MONOTONIC);
	g_assert_cmpint(gpiod_line_settings_get_output_value(settings), ==,
			GPIOD_LINE_VALUE_INACTIVE);
}

GPIOD_TEST_CASE(set_direction)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	gint ret;

	settings = gpiod_test_create_line_settings_or_fail();

	ret = gpiod_line_settings_set_direction(settings,
						GPIOD_LINE_DIRECTION_INPUT);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_direction(settings), ==,
			GPIOD_LINE_DIRECTION_INPUT);

	ret = gpiod_line_settings_set_direction(settings,
						GPIOD_LINE_DIRECTION_AS_IS);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_direction(settings), ==,
			GPIOD_LINE_DIRECTION_AS_IS);

	ret = gpiod_line_settings_set_direction(settings,
						GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_direction(settings), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);

	ret = gpiod_line_settings_set_direction(settings, 999);
	g_assert_cmpint(ret, <, 0);
	g_assert_cmpint(errno, ==, EINVAL);
	g_assert_cmpint(gpiod_line_settings_get_direction(settings), ==,
			GPIOD_LINE_DIRECTION_AS_IS);
}

GPIOD_TEST_CASE(set_edge_detection)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	gint ret;

	settings = gpiod_test_create_line_settings_or_fail();

	ret = gpiod_line_settings_set_edge_detection(settings,
						     GPIOD_LINE_EDGE_BOTH);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_edge_detection(settings), ==,
			GPIOD_LINE_EDGE_BOTH);

	ret = gpiod_line_settings_set_edge_detection(settings,
						     GPIOD_LINE_EDGE_NONE);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_edge_detection(settings), ==,
			GPIOD_LINE_EDGE_NONE);

	ret = gpiod_line_settings_set_edge_detection(settings,
						     GPIOD_LINE_EDGE_FALLING);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_edge_detection(settings), ==,
			GPIOD_LINE_EDGE_FALLING);

	ret = gpiod_line_settings_set_edge_detection(settings,
						     GPIOD_LINE_EDGE_RISING);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_edge_detection(settings), ==,
			GPIOD_LINE_EDGE_RISING);

	ret = gpiod_line_settings_set_edge_detection(settings, 999);
	g_assert_cmpint(ret, <, 0);
	g_assert_cmpint(errno, ==, EINVAL);
	g_assert_cmpint(gpiod_line_settings_get_edge_detection(settings), ==,
			GPIOD_LINE_EDGE_NONE);
}

GPIOD_TEST_CASE(set_bias)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	gint ret;

	settings = gpiod_test_create_line_settings_or_fail();

	ret = gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_DISABLED);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_bias(settings), ==,
			GPIOD_LINE_BIAS_DISABLED);

	ret = gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_AS_IS);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_bias(settings), ==,
			GPIOD_LINE_BIAS_AS_IS);

	ret = gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_DOWN);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_bias(settings), ==,
			GPIOD_LINE_BIAS_PULL_DOWN);

	ret = gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_bias(settings), ==,
			GPIOD_LINE_BIAS_PULL_UP);

	ret = gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_UNKNOWN);
	g_assert_cmpint(ret, <, 0);
	g_assert_cmpint(errno, ==, EINVAL);
	g_assert_cmpint(gpiod_line_settings_get_bias(settings), ==,
			GPIOD_LINE_BIAS_AS_IS);

	ret = gpiod_line_settings_set_bias(settings, 999);
	g_assert_cmpint(ret, <, 0);
	g_assert_cmpint(errno, ==, EINVAL);
	g_assert_cmpint(gpiod_line_settings_get_bias(settings), ==,
			GPIOD_LINE_BIAS_AS_IS);
}

GPIOD_TEST_CASE(set_drive)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	gint ret;

	settings = gpiod_test_create_line_settings_or_fail();

	ret = gpiod_line_settings_set_drive(settings,
					    GPIOD_LINE_DRIVE_OPEN_DRAIN);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_drive(settings), ==,
			GPIOD_LINE_DRIVE_OPEN_DRAIN);

	ret = gpiod_line_settings_set_drive(settings,
					    GPIOD_LINE_DRIVE_PUSH_PULL);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_drive(settings), ==,
			GPIOD_LINE_DRIVE_PUSH_PULL);

	ret = gpiod_line_settings_set_drive(settings,
					    GPIOD_LINE_DRIVE_OPEN_SOURCE);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_drive(settings), ==,
			GPIOD_LINE_DRIVE_OPEN_SOURCE);

	ret = gpiod_line_settings_set_drive(settings, 999);
	g_assert_cmpint(ret, <, 0);
	g_assert_cmpint(errno, ==, EINVAL);
	g_assert_cmpint(gpiod_line_settings_get_drive(settings), ==,
			GPIOD_LINE_DRIVE_PUSH_PULL);
}

GPIOD_TEST_CASE(set_active_low)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;

	settings = gpiod_test_create_line_settings_or_fail();

	gpiod_line_settings_set_active_low(settings, true);
	g_assert_true(gpiod_line_settings_get_active_low(settings));

	gpiod_line_settings_set_active_low(settings, false);
	g_assert_false(gpiod_line_settings_get_active_low(settings));
}

GPIOD_TEST_CASE(set_debounce_period)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;

	settings = gpiod_test_create_line_settings_or_fail();

	gpiod_line_settings_set_debounce_period_us(settings, 4000);
	g_assert_cmpint(gpiod_line_settings_get_debounce_period_us(settings),
			==, 4000);
}

GPIOD_TEST_CASE(set_event_clock)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	gint ret;

	settings = gpiod_test_create_line_settings_or_fail();

	ret = gpiod_line_settings_set_event_clock(settings,
					GPIOD_LINE_CLOCK_MONOTONIC);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_event_clock(settings), ==,
			GPIOD_LINE_CLOCK_MONOTONIC);

	ret = gpiod_line_settings_set_event_clock(settings,
					GPIOD_LINE_CLOCK_REALTIME);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_event_clock(settings), ==,
			GPIOD_LINE_CLOCK_REALTIME);

	ret = gpiod_line_settings_set_event_clock(settings,
					GPIOD_LINE_CLOCK_HTE);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_event_clock(settings), ==,
			GPIOD_LINE_CLOCK_HTE);

	ret = gpiod_line_settings_set_event_clock(settings, 999);
	g_assert_cmpint(ret, <, 0);
	g_assert_cmpint(errno, ==, EINVAL);
	g_assert_cmpint(gpiod_line_settings_get_event_clock(settings), ==,
			GPIOD_LINE_CLOCK_MONOTONIC);
}

GPIOD_TEST_CASE(set_output_value)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	gint ret;

	settings = gpiod_test_create_line_settings_or_fail();

	ret = gpiod_line_settings_set_output_value(settings,
						   GPIOD_LINE_VALUE_ACTIVE);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_output_value(settings), ==,
			GPIOD_LINE_VALUE_ACTIVE);

	ret = gpiod_line_settings_set_output_value(settings,
						   GPIOD_LINE_VALUE_INACTIVE);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_output_value(settings), ==,
			GPIOD_LINE_VALUE_INACTIVE);

	ret = gpiod_line_settings_set_output_value(settings, 999);
	g_assert_cmpint(ret, <, 0);
	g_assert_cmpint(errno, ==, EINVAL);
	g_assert_cmpint(gpiod_line_settings_get_output_value(settings), ==,
			GPIOD_LINE_VALUE_INACTIVE);
}

GPIOD_TEST_CASE(copy_line_settings)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_settings) copy = NULL;

	settings = gpiod_test_create_line_settings_or_fail();

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
	gpiod_line_settings_set_debounce_period_us(settings, 2000);
	gpiod_line_settings_set_event_clock(settings,
					    GPIOD_LINE_CLOCK_REALTIME);

	copy = gpiod_line_settings_copy(settings);
	g_assert_nonnull(copy);
	gpiod_test_return_if_failed();
	g_assert_false(settings == copy);
	g_assert_cmpint(gpiod_line_settings_get_direction(copy), ==,
			GPIOD_LINE_DIRECTION_INPUT);
	g_assert_cmpint(gpiod_line_settings_get_edge_detection(copy), ==,
			GPIOD_LINE_EDGE_BOTH);
	g_assert_cmpint(gpiod_line_settings_get_debounce_period_us(copy), ==,
			2000);
	g_assert_cmpint(gpiod_line_settings_get_event_clock(copy), ==,
			GPIOD_LINE_CLOCK_REALTIME);
}

GPIOD_TEST_CASE(reset_settings)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;

	settings = gpiod_test_create_line_settings_or_fail();

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
	gpiod_line_settings_set_debounce_period_us(settings, 2000);
	gpiod_line_settings_set_event_clock(settings,
					    GPIOD_LINE_CLOCK_REALTIME);

	gpiod_line_settings_reset(settings);

	g_assert_cmpint(gpiod_line_settings_get_direction(settings), ==,
			GPIOD_LINE_DIRECTION_AS_IS);
	g_assert_cmpint(gpiod_line_settings_get_edge_detection(settings), ==,
			GPIOD_LINE_EDGE_NONE);
	g_assert_cmpint(gpiod_line_settings_get_bias(settings), ==,
			GPIOD_LINE_BIAS_AS_IS);
	g_assert_cmpint(gpiod_line_settings_get_drive(settings), ==,
			GPIOD_LINE_DRIVE_PUSH_PULL);
	g_assert_false(gpiod_line_settings_get_active_low(settings));
	g_assert_cmpuint(gpiod_line_settings_get_debounce_period_us(settings),
			 ==, 0);
	g_assert_cmpint(gpiod_line_settings_get_event_clock(settings), ==,
			GPIOD_LINE_CLOCK_MONOTONIC);
	g_assert_cmpint(gpiod_line_settings_get_output_value(settings), ==,
			GPIOD_LINE_VALUE_INACTIVE);
}
