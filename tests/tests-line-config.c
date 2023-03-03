// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <errno.h>
#include <glib.h>
#include <gpiod.h>

#include "gpiod-test.h"
#include "gpiod-test-helpers.h"
#include "gpiod-test-sim.h"

#define GPIOD_TEST_GROUP "line-config"

GPIOD_TEST_CASE(too_many_lines)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) config = NULL;
	guint offsets[65], i;
	gint ret;

	settings = gpiod_test_create_line_settings_or_fail();
	config = gpiod_test_create_line_config_or_fail();

	for (i = 0; i < 65; i++)
		offsets[i] = i;

	ret = gpiod_line_config_add_line_settings(config, offsets, 65,
						  settings);
	g_assert_cmpint(ret, <, 0);
	g_assert_cmpint(errno, ==, E2BIG);
}

GPIOD_TEST_CASE(get_line_settings)
{
	static const guint offsets[] = { 0, 1, 2, 3 };

	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_settings) retrieved = NULL;
	g_autoptr(struct_gpiod_line_config) config = NULL;

	settings = gpiod_test_create_line_settings_or_fail();
	config = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_DOWN);
	gpiod_test_line_config_add_line_settings_or_fail(config, offsets, 4,
							 settings);

	retrieved = gpiod_test_line_config_get_line_settings_or_fail(config, 2);

	g_assert_cmpint(gpiod_line_settings_get_direction(settings), ==,
			GPIOD_LINE_DIRECTION_INPUT);
	g_assert_cmpint(gpiod_line_settings_get_bias(settings), ==,
			GPIOD_LINE_BIAS_PULL_DOWN);
}

GPIOD_TEST_CASE(too_many_attrs)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	guint offset;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_ACTIVE);
	offset = 0;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_line_settings_set_debounce_period_us(settings, 1000);
	gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
	offset = 1;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
	offset = 2;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_DOWN);
	offset = 3;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_DISABLED);
	offset = 4;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	gpiod_line_settings_set_active_low(settings, true);
	offset = 5;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	gpiod_line_settings_set_edge_detection(settings,
					       GPIOD_LINE_EDGE_FALLING);
	offset = 6;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	gpiod_line_settings_set_event_clock(settings,
					    GPIOD_LINE_CLOCK_REALTIME);
	offset = 7;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	gpiod_line_settings_reset(settings);

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_drive(settings, GPIOD_LINE_DRIVE_OPEN_DRAIN);
	offset = 8;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	gpiod_line_settings_set_drive(settings, GPIOD_LINE_DRIVE_OPEN_SOURCE);
	offset = 9;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	request = gpiod_chip_request_lines(chip, NULL, line_cfg);
	g_assert_null(request);
	g_assert_cmpint(errno, ==, E2BIG);
}

GPIOD_TEST_CASE(null_settings)
{
	static const guint offsets[] = { 0, 1, 2, 3 };

	g_autoptr(struct_gpiod_line_config) config = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;

	config = gpiod_test_create_line_config_or_fail();

	gpiod_test_line_config_add_line_settings_or_fail(config, offsets, 4,
							 NULL);

	settings = gpiod_test_line_config_get_line_settings_or_fail(config, 2);

	g_assert_cmpint(gpiod_line_settings_get_drive(settings), ==,
			GPIOD_LINE_DIRECTION_AS_IS);
}

GPIOD_TEST_CASE(reset_config)
{
	static const guint offsets[] = { 0, 1, 2, 3 };

	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_settings) retrieved0 = NULL;
	g_autoptr(struct_gpiod_line_settings) retrieved1 = NULL;
	g_autoptr(struct_gpiod_line_config) config = NULL;

	settings = gpiod_test_create_line_settings_or_fail();
	config = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_DOWN);
	gpiod_test_line_config_add_line_settings_or_fail(config, offsets, 4,
							 settings);

	retrieved0 = gpiod_test_line_config_get_line_settings_or_fail(config,
								      2);

	g_assert_cmpint(gpiod_line_settings_get_direction(retrieved0), ==,
			GPIOD_LINE_DIRECTION_INPUT);
	g_assert_cmpint(gpiod_line_settings_get_bias(retrieved0), ==,
			GPIOD_LINE_BIAS_PULL_DOWN);

	gpiod_line_config_reset(config);

	retrieved1 = gpiod_line_config_get_line_settings(config, 2);
	g_assert_null(retrieved1);
}

GPIOD_TEST_CASE(get_offsets)
{
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) config = NULL;
	guint offsets[8], offsets_in[4];
	size_t num_offsets;

	settings = gpiod_test_create_line_settings_or_fail();
	config = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_DOWN);
	offsets[0] = 2;
	offsets[1] = 4;
	gpiod_test_line_config_add_line_settings_or_fail(config, offsets, 2,
							 settings);

	gpiod_line_settings_set_edge_detection(settings, GPIOD_LINE_EDGE_BOTH);
	offsets[0] = 6;
	offsets[1] = 7;
	gpiod_test_line_config_add_line_settings_or_fail(config, offsets, 2,
							 settings);

	num_offsets = gpiod_line_config_get_configured_offsets(config,
							       offsets_in, 4);
	g_assert_cmpuint(num_offsets, ==, 4);
	g_assert_cmpuint(offsets_in[0], ==, 2);
	g_assert_cmpuint(offsets_in[1], ==, 4);
	g_assert_cmpuint(offsets_in[2], ==, 6);
	g_assert_cmpuint(offsets_in[3], ==, 7);
}

GPIOD_TEST_CASE(get_0_offsets)
{
	g_autoptr(struct_gpiod_line_config) config = NULL;
	size_t num_offsets;
	guint offsets[3];

	config = gpiod_test_create_line_config_or_fail();

	num_offsets = gpiod_line_config_get_configured_offsets(config,
							       offsets, 0);
	g_assert_cmpuint(num_offsets, ==, 0);
}

GPIOD_TEST_CASE(get_null_offsets)
{
	g_autoptr(struct_gpiod_line_config) config = NULL;
	size_t num_offsets;

	config = gpiod_test_create_line_config_or_fail();

	num_offsets = gpiod_line_config_get_configured_offsets(config,
							       NULL, 10);
	g_assert_cmpuint(num_offsets, ==, 0);
}

GPIOD_TEST_CASE(get_less_offsets_than_configured)
{
	static const guint offsets[] = { 0, 1, 2, 3 };

	g_autoptr(struct_gpiod_line_config) config = NULL;
	size_t num_retrieved;
	guint retrieved[3];

	config = gpiod_test_create_line_config_or_fail();

	gpiod_test_line_config_add_line_settings_or_fail(config, offsets, 4,
							 NULL);

	num_retrieved = gpiod_line_config_get_configured_offsets(config,
								 retrieved, 3);
	g_assert_cmpuint(num_retrieved, ==, 3);
	g_assert_cmpuint(retrieved[0], ==, 0);
	g_assert_cmpuint(retrieved[1], ==, 1);
	g_assert_cmpuint(retrieved[2], ==, 2);
}

GPIOD_TEST_CASE(get_more_offsets_than_configured)
{
	static const guint offsets[] = { 0, 1, 2, 3 };

	g_autoptr(struct_gpiod_line_config) config = NULL;
	size_t num_retrieved;
	guint retrieved[8];

	config = gpiod_test_create_line_config_or_fail();

	gpiod_test_line_config_add_line_settings_or_fail(config, offsets, 4,
							 NULL);

	num_retrieved = gpiod_line_config_get_configured_offsets(config,
								 retrieved, 8);
	g_assert_cmpuint(num_retrieved, ==, 4);
	g_assert_cmpuint(retrieved[0], ==, 0);
	g_assert_cmpuint(retrieved[1], ==, 1);
	g_assert_cmpuint(retrieved[2], ==, 2);
	g_assert_cmpuint(retrieved[3], ==, 3);
}

GPIOD_TEST_CASE(set_global_output_values)
{
	static const guint offsets[] = { 0, 1, 2, 3 };
	static const enum gpiod_line_value values[] = {
		GPIOD_LINE_VALUE_ACTIVE,
		GPIOD_LINE_VALUE_INACTIVE,
		GPIOD_LINE_VALUE_ACTIVE,
		GPIOD_LINE_VALUE_INACTIVE,
	};

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 4, NULL);
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) config = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	g_autoptr(struct_gpiod_chip) chip = NULL;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	config = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_test_line_config_add_line_settings_or_fail(config, offsets, 4,
							 settings);
	gpiod_test_line_config_set_output_values_or_fail(config, values, 4);

	request = gpiod_test_request_lines_or_fail(chip, NULL, config);

	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 0), ==,
			G_GPIOSIM_VALUE_ACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 1), ==,
			G_GPIOSIM_VALUE_INACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 2), ==,
			G_GPIOSIM_VALUE_ACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 3), ==,
			G_GPIOSIM_VALUE_INACTIVE);
}

GPIOD_TEST_CASE(read_back_global_output_values)
{
	static const guint offsets[] = { 0, 1, 2, 3 };
	static const enum gpiod_line_value values[] = {
		GPIOD_LINE_VALUE_ACTIVE,
		GPIOD_LINE_VALUE_INACTIVE,
		GPIOD_LINE_VALUE_ACTIVE,
		GPIOD_LINE_VALUE_INACTIVE,
	};

	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_settings) retrieved = NULL;
	g_autoptr(struct_gpiod_line_config) config = NULL;

	settings = gpiod_test_create_line_settings_or_fail();
	config = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_ACTIVE);
	gpiod_test_line_config_add_line_settings_or_fail(config, offsets, 4,
							 settings);
	gpiod_test_line_config_set_output_values_or_fail(config, values, 4);

	retrieved = gpiod_test_line_config_get_line_settings_or_fail(config, 1);
	g_assert_cmpint(gpiod_line_settings_get_output_value(retrieved), ==,
			GPIOD_LINE_VALUE_INACTIVE);
}

GPIOD_TEST_CASE(set_output_values_invalid_value)
{
	static const enum gpiod_line_value values[] = {
		GPIOD_LINE_VALUE_ACTIVE,
		GPIOD_LINE_VALUE_INACTIVE,
		999,
		GPIOD_LINE_VALUE_INACTIVE,
	};

	g_autoptr(struct_gpiod_line_config) config = NULL;

	config = gpiod_test_create_line_config_or_fail();

	g_assert_cmpint(gpiod_line_config_set_output_values(config, values, 4),
			==, -1);
	gpiod_test_expect_errno(EINVAL);
}
