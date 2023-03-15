// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <glib.h>
#include <gpiod.h>

#include "gpiod-test.h"
#include "gpiod-test-helpers.h"
#include "gpiod-test-sim.h"

#define GPIOD_TEST_GROUP "line-request"

GPIOD_TEST_CASE(request_fails_with_no_offsets)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 4, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;

	line_cfg = gpiod_test_create_line_config_or_fail();

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));

	request = gpiod_chip_request_lines(chip, NULL, line_cfg);
	g_assert_null(request);
	gpiod_test_expect_errno(EINVAL);
}

GPIOD_TEST_CASE(request_fails_with_no_line_config)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 4, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));

	request = gpiod_chip_request_lines(chip, NULL, NULL);
	g_assert_null(request);
	gpiod_test_expect_errno(EINVAL);
}

GPIOD_TEST_CASE(request_fails_with_duplicate_offsets)
{
	static const guint offsets[] = { 0, 2, 2, 3 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 4, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	size_t num_requested_offsets;
	guint requested_offsets[3];

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 4,
							 NULL);

	request = gpiod_chip_request_lines(chip, NULL, line_cfg);
	g_assert_nonnull(request);
	num_requested_offsets =
			gpiod_line_request_get_num_requested_lines(request);
	g_assert_cmpuint(num_requested_offsets, ==, 3);
	gpiod_line_request_get_requested_offsets(request, requested_offsets, 4);
	g_assert_cmpuint(requested_offsets[0], ==, 0);
	g_assert_cmpuint(requested_offsets[1], ==, 2);
	g_assert_cmpuint(requested_offsets[2], ==, 3);
}

GPIOD_TEST_CASE(request_fails_with_offset_out_of_bounds)
{
	static const guint offsets[] = { 2, 6 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 4, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 2,
							 NULL);

	request = gpiod_chip_request_lines(chip, NULL, line_cfg);
	g_assert_null(request);
	gpiod_test_expect_errno(EINVAL);
}

GPIOD_TEST_CASE(set_consumer)
{
	static const guint offset = 2;
	static const gchar *const consumer = "foobar";

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 4, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_request_config) req_cfg = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	g_autoptr(struct_gpiod_line_info) info = NULL;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	req_cfg = gpiod_test_create_request_config_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_request_config_set_consumer(req_cfg, consumer);

	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 NULL);

	request = gpiod_test_request_lines_or_fail(chip, req_cfg, line_cfg);

	info = gpiod_test_get_line_info_or_fail(chip, offset);

	g_assert_true(gpiod_line_info_is_used(info));
	g_assert_cmpstr(gpiod_line_info_get_consumer(info), ==, consumer);
}

GPIOD_TEST_CASE(empty_consumer)
{
	static const guint offset = 2;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 4, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	g_autoptr(struct_gpiod_line_info) info = NULL;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 NULL);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	info = gpiod_test_get_line_info_or_fail(chip, offset);

	g_assert_cmpstr(gpiod_line_info_get_consumer(info), ==, "?");
}

GPIOD_TEST_CASE(default_output_value)
{
	/*
	 * Have a hole in offsets on purpose - make sure it's not set by
	 * accident.
	 */
	static const guint offsets[] = { 0, 1, 3, 4 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	guint i;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_ACTIVE);

	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 4,
							 settings);

	g_gpiosim_chip_set_pull(sim, 2, G_GPIOSIM_PULL_DOWN);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	for (i = 0; i < 4; i++)
		g_assert_cmpint(g_gpiosim_chip_get_value(sim, offsets[i]), ==,
				G_GPIOSIM_VALUE_ACTIVE);

	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 2), ==,
			G_GPIOSIM_VALUE_INACTIVE);
}

GPIOD_TEST_CASE(read_all_values)
{
	static const guint offsets[] = { 0, 2, 4, 5, 7 };
	static const gint pulls[] = { 0, 1, 0, 1, 1 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	enum gpiod_line_value values[5];
	gint ret;
	guint i;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 5,
							 settings);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	for (i = 0; i < 5; i++)
		g_gpiosim_chip_set_pull(sim, offsets[i],
					pulls[i] ? G_GPIOSIM_PULL_UP :
						   G_GPIOSIM_PULL_DOWN);

	ret = gpiod_line_request_get_values(request, values);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	for (i = 0; i < 5; i++)
		g_assert_cmpint(values[i], ==, pulls[i]);
}

GPIOD_TEST_CASE(request_multiple_values_but_read_one)
{
	static const guint offsets[] = { 0, 2, 4, 5, 7 };
	static const gint pulls[] = { 0, 1, 0, 1, 1 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	gint ret;
	guint i;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 5,
							 settings);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	for (i = 0; i < 5; i++)
		g_gpiosim_chip_set_pull(sim, offsets[i],
					pulls[i] ? G_GPIOSIM_PULL_UP :
						   G_GPIOSIM_PULL_DOWN);

	ret = gpiod_line_request_get_value(request, 5);
	g_assert_cmpint(ret, ==, 1);
}

GPIOD_TEST_CASE(set_all_values)
{
	static const guint offsets[] = { 0, 2, 4, 5, 6 };
	static const enum gpiod_line_value values[] = {
		GPIOD_LINE_VALUE_ACTIVE,
		GPIOD_LINE_VALUE_INACTIVE,
		GPIOD_LINE_VALUE_ACTIVE,
		GPIOD_LINE_VALUE_ACTIVE,
		GPIOD_LINE_VALUE_ACTIVE
	};
	static const GPIOSimValue sim_values[] = {
		G_GPIOSIM_VALUE_ACTIVE,
		G_GPIOSIM_VALUE_INACTIVE,
		G_GPIOSIM_VALUE_ACTIVE,
		G_GPIOSIM_VALUE_ACTIVE,
		G_GPIOSIM_VALUE_ACTIVE
	};

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	gint ret;
	guint i;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 5,
							 settings);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	ret = gpiod_line_request_set_values(request, values);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	for (i = 0; i < 5; i++)
		g_assert_cmpint(g_gpiosim_chip_get_value(sim, offsets[i]), ==,
				sim_values[i]);
}

GPIOD_TEST_CASE(set_values_subset_of_lines)
{
	static const guint offsets[] = { 0, 1, 2, 3 };
	static const guint offsets_to_set[] = { 0, 1, 3 };
	static const enum gpiod_line_value values[] = {
		GPIOD_LINE_VALUE_ACTIVE,
		GPIOD_LINE_VALUE_INACTIVE,
		GPIOD_LINE_VALUE_ACTIVE
	};

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 4, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	gint ret;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 4,
							 settings);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	ret = gpiod_line_request_set_values_subset(request, 3, offsets_to_set,
						   values);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 0), ==,
			G_GPIOSIM_VALUE_ACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 1), ==,
			G_GPIOSIM_VALUE_INACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 3), ==,
			G_GPIOSIM_VALUE_ACTIVE);
}

GPIOD_TEST_CASE(set_line_after_requesting)
{
	static const guint offsets[] = { 0, 1, 3, 4 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_output_value(settings,
					     GPIOD_LINE_VALUE_INACTIVE);
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 4,
							 settings);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	gpiod_line_request_set_value(request, 1, GPIOD_LINE_VALUE_ACTIVE);

	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 0), ==,
			G_GPIOSIM_VALUE_INACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 1), ==,
			G_GPIOSIM_VALUE_ACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 3), ==,
			G_GPIOSIM_VALUE_INACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 4), ==,
			G_GPIOSIM_VALUE_INACTIVE);
}

GPIOD_TEST_CASE(request_survives_parent_chip)
{
	static const guint offset = 0;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 4, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	gint ret;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_ACTIVE);
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	g_assert_cmpint(gpiod_line_request_get_value(request, offset), ==,
			GPIOD_LINE_VALUE_ACTIVE);

	gpiod_chip_close(chip);
	chip = NULL;

	ret = gpiod_line_request_set_value(request, offset,
					   GPIOD_LINE_VALUE_ACTIVE);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_set_value(request, offset,
					   GPIOD_LINE_VALUE_ACTIVE);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
}

GPIOD_TEST_CASE(num_lines_and_offsets)
{
	static const guint offsets[] = { 0, 1, 2, 3, 7, 8, 11, 14 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 16, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	guint read_back[8], i;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 8,
							 NULL);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	g_assert_cmpuint(gpiod_line_request_get_num_requested_lines(request),
			 ==, 8);
	gpiod_test_return_if_failed();
	gpiod_line_request_get_requested_offsets(request, read_back, 8);
	for (i = 0; i < 8; i++)
		g_assert_cmpuint(read_back[i], ==, offsets[i]);
}

GPIOD_TEST_CASE(active_low_read_value)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	guint offset;
	gint value;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_active_low(settings, true);
	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	offset = 2;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);
	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_ACTIVE);
	offset = 3;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	g_gpiosim_chip_set_pull(sim, 2, G_GPIOSIM_PULL_DOWN);
	value = gpiod_line_request_get_value(request, 2);
	g_assert_cmpint(value, ==, GPIOD_LINE_VALUE_ACTIVE);

	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 3), ==,
			G_GPIOSIM_VALUE_INACTIVE);
}

GPIOD_TEST_CASE(reconfigure_lines)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 4, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	guint offsets[2];
	gint ret;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);

	gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_ACTIVE);
	offsets[0] = 0;
	offsets[1] = 2;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 2,
							 settings);
	gpiod_line_settings_set_output_value(settings,
					     GPIOD_LINE_VALUE_INACTIVE);
	offsets[0] = 1;
	offsets[1] = 3;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 2,
							 settings);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 0), ==,
			G_GPIOSIM_VALUE_ACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 1), ==,
			G_GPIOSIM_VALUE_INACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 2), ==,
			G_GPIOSIM_VALUE_ACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 3), ==,
			G_GPIOSIM_VALUE_INACTIVE);

	gpiod_line_config_reset(line_cfg);

	gpiod_line_settings_set_output_value(settings,
					     GPIOD_LINE_VALUE_INACTIVE);
	offsets[0] = 0;
	offsets[1] = 2;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 2,
							 settings);
	gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_ACTIVE);
	offsets[0] = 1;
	offsets[1] = 3;
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 2,
							 settings);

	ret = gpiod_line_request_reconfigure_lines(request, line_cfg);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 0), ==,
			G_GPIOSIM_VALUE_INACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 1), ==,
			G_GPIOSIM_VALUE_ACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 2), ==,
			G_GPIOSIM_VALUE_INACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 3), ==,
			G_GPIOSIM_VALUE_ACTIVE);
}

GPIOD_TEST_CASE(reconfigure_lines_null_config)
{
	static const guint offsets[] = { 0, 1, 2, 3 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	gint ret;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 4,
							 NULL);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	ret = gpiod_line_request_reconfigure_lines(request, NULL);
	g_assert_cmpint(ret, ==, -1);
	gpiod_test_expect_errno(EINVAL);
}

GPIOD_TEST_CASE(reconfigure_lines_different_offsets)
{
	static const guint offsets0[] = { 0, 1, 2, 3 };
	static const guint offsets1[] = { 2, 4, 5 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	gint ret;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets0, 4,
							 NULL);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	gpiod_line_config_reset(line_cfg);

	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets1, 3,
							 NULL);

	ret = gpiod_line_request_reconfigure_lines(request, NULL);
	g_assert_cmpint(ret, ==, -1);
	gpiod_test_expect_errno(EINVAL);
}

GPIOD_TEST_CASE(request_lines_with_unordered_offsets)
{
	static const guint offsets[] = { 5, 1, 7, 2, 0, 6 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	enum gpiod_line_value values[4];
	guint set_offsets[4];

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings,
					  GPIOD_LINE_DIRECTION_OUTPUT);
	gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_ACTIVE);

	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 6,
							 settings);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	values[0] = 0;
	values[1] = 1;
	values[2] = 0;
	values[3] = 0;
	set_offsets[0] = 7;
	set_offsets[1] = 1;
	set_offsets[2] = 6;
	set_offsets[3] = 0;
	gpiod_line_request_set_values_subset(request, 4, set_offsets, values);

	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 0), ==,
			G_GPIOSIM_VALUE_INACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 1), ==,
			G_GPIOSIM_VALUE_ACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 2), ==,
			G_GPIOSIM_VALUE_ACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 5), ==,
			G_GPIOSIM_VALUE_ACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 6), ==,
			G_GPIOSIM_VALUE_INACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 7), ==,
			G_GPIOSIM_VALUE_INACTIVE);
}

GPIOD_TEST_CASE(request_with_bias_set_to_pull_up)
{
	static const guint offset = 3;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_settings) settings = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	settings = gpiod_test_create_line_settings_or_fail();
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
	gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, &offset, 1,
							 settings);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 3), ==,
			G_GPIOSIM_VALUE_ACTIVE);
}

GPIOD_TEST_CASE(get_requested_offsets_less_and_more)
{
	static const guint offsets[] = { 0, 1, 2, 3 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(struct_gpiod_chip) chip = NULL;
	g_autoptr(struct_gpiod_line_config) line_cfg = NULL;
	g_autoptr(struct_gpiod_line_request) request = NULL;
	size_t num_retrieved;
	guint retrieved[6];

	chip = gpiod_test_open_chip_or_fail(g_gpiosim_chip_get_dev_path(sim));
	line_cfg = gpiod_test_create_line_config_or_fail();

	gpiod_test_line_config_add_line_settings_or_fail(line_cfg, offsets, 4,
							 NULL);

	request = gpiod_test_request_lines_or_fail(chip, NULL, line_cfg);

	num_retrieved = gpiod_line_request_get_requested_offsets(request,
								 retrieved, 3);

	g_assert_cmpuint(num_retrieved, ==, 3);
	g_assert_cmpuint(retrieved[0], ==, 0);
	g_assert_cmpuint(retrieved[1], ==, 1);
	g_assert_cmpuint(retrieved[2], ==, 2);

	memset(retrieved, 0, sizeof(retrieved));

	num_retrieved = gpiod_line_request_get_requested_offsets(request,
								 retrieved, 6);

	g_assert_cmpuint(num_retrieved, ==, 4);
	g_assert_cmpuint(retrieved[0], ==, 0);
	g_assert_cmpuint(retrieved[1], ==, 1);
	g_assert_cmpuint(retrieved[2], ==, 2);
	g_assert_cmpuint(retrieved[3], ==, 3);
}
