// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2023-2024 Bartosz Golaszewski <bartosz.golaszewski@linaro.org>

#include <glib.h>
#include <gpiod-glib.h>
#include <gpiod-test.h>
#include <gpiosim-glib.h>

#include "helpers.h"

#define GPIOD_TEST_GROUP "glib/line-request"

GPIOD_TEST_CASE(request_fails_with_no_offsets)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GError) err = NULL;

	line_cfg = gpiodglib_line_config_new();

	chip = gpiodglib_test_new_chip_or_fail(
			g_gpiosim_chip_get_dev_path(sim));

	request = gpiodglib_chip_request_lines(chip, NULL, line_cfg, &err);
	g_assert_null(request);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL);
}

GPIOD_TEST_CASE(request_fails_with_no_line_config)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GError) err = NULL;

	chip = gpiodglib_test_new_chip_or_fail(
			g_gpiosim_chip_get_dev_path(sim));

	request = gpiodglib_chip_request_lines(chip, NULL, NULL, &err);
	g_assert_null(request);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL);
}

GPIOD_TEST_CASE(set_consumer)
{
	static const gchar *const consumer = "foobar";
	static const guint offset = 2;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GpiodglibRequestConfig) req_cfg = NULL;
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GpiodglibLineInfo) info = NULL;
	g_autofree gchar *cpy = NULL;

	chip = gpiodglib_test_new_chip_or_fail(
			g_gpiosim_chip_get_dev_path(sim));

	req_cfg = gpiodglib_request_config_new("consumer", consumer, NULL);
	line_cfg = gpiodglib_line_config_new();
	offsets = gpiodglib_test_array_from_const(&offset, 1, sizeof(guint));

	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets, NULL);

	request = gpiodglib_test_chip_request_lines_or_fail(chip, req_cfg,
							    line_cfg);

	info = gpiodglib_test_chip_get_line_info_or_fail(chip, offset);
	cpy = gpiodglib_line_info_dup_consumer(info);
	g_assert_cmpstr(cpy, ==, consumer);
}

GPIOD_TEST_CASE(empty_consumer)
{
	static const guint offset = 2;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibChip) chip = NULL;
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GpiodglibLineInfo) info = NULL;
	g_autofree gchar *consumer = NULL;

	chip = gpiodglib_test_new_chip_or_fail(
			g_gpiosim_chip_get_dev_path(sim));

	line_cfg = gpiodglib_line_config_new();
	offsets = gpiodglib_test_array_from_const(&offset, 1, sizeof(guint));

	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets, NULL);

	request = gpiodglib_test_chip_request_lines_or_fail(chip, NULL,
							    line_cfg);

	info = gpiodglib_test_chip_get_line_info_or_fail(chip, offset);
	consumer = gpiodglib_line_info_dup_consumer(info);
	g_assert_cmpstr(consumer, ==, "?");
}

GPIOD_TEST_CASE(get_requested_offsets)
{
	static const guint offset_vals[] = { 2, 1, 6, 4 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GArray) retrieved = NULL;

	line_cfg = gpiodglib_line_config_new();
	offsets = gpiodglib_test_array_from_const(offset_vals, 4,
						  sizeof(guint));

	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets, NULL);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	retrieved = gpiodglib_line_request_get_requested_offsets(request);
	g_assert_cmpuint(retrieved->len, ==, 4);
	g_assert_cmpuint(g_array_index(retrieved, guint, 0), ==, 2);
	g_assert_cmpuint(g_array_index(retrieved, guint, 1), ==, 1);
	g_assert_cmpuint(g_array_index(retrieved, guint, 2), ==, 6);
	g_assert_cmpuint(g_array_index(retrieved, guint, 3), ==, 4);
}

GPIOD_TEST_CASE(released_request_cannot_be_used_reconfigure)
{
	static const guint offset = 3;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;

	line_cfg = gpiodglib_line_config_new();
	offsets = gpiodglib_test_array_from_const(&offset, 1, sizeof(guint));

	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets, NULL);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	gpiodglib_line_request_release(request);

	ret = gpiodglib_line_request_reconfigure_lines(request, line_cfg, &err);
	g_assert_false(ret);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_REQUEST_RELEASED);
}

GPIOD_TEST_CASE(released_request_cannot_be_used_get_value)
{
	static const guint offset = 3;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GArray) values = NULL;
	g_autoptr(GError) err = NULL;
	GpiodglibLineValue value;
	gboolean ret;

	line_cfg = gpiodglib_line_config_new();
	settings = gpiodglib_line_settings_new(
			"direction", GPIODGLIB_LINE_DIRECTION_INPUT, NULL);
	offsets = gpiodglib_test_array_from_const(&offset, 1, sizeof(guint));
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets, NULL);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	gpiodglib_line_request_release(request);

	ret = gpiodglib_line_request_get_value(request, offset, &value, &err);
	g_assert_false(ret);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_REQUEST_RELEASED);

	g_clear_pointer(&err, g_error_free);

	ret = gpiodglib_line_request_get_values(request, &values, &err);
	g_assert_false(ret);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_REQUEST_RELEASED);
}

GPIOD_TEST_CASE(released_request_cannot_be_used_set_value)
{
	static const guint offset = 3;
	static const GpiodglibLineValue value = GPIODGLIB_LINE_VALUE_ACTIVE;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GArray) values = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;

	line_cfg = gpiodglib_line_config_new();
	settings = gpiodglib_line_settings_new(
			"direction", GPIODGLIB_LINE_DIRECTION_OUTPUT, NULL);
	offsets = gpiodglib_test_array_from_const(&offset, 1, sizeof(guint));
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets, NULL);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	gpiodglib_line_request_release(request);

	ret = gpiodglib_line_request_set_value(request, offset, value, &err);
	g_assert_false(ret);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_REQUEST_RELEASED);

	g_clear_pointer(&err, g_error_free);

	values = gpiodglib_test_array_from_const(&value, 1, sizeof(value));
	ret = gpiodglib_line_request_set_values(request, values, &err);
	g_assert_false(ret);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_REQUEST_RELEASED);
}

GPIOD_TEST_CASE(reconfigure_lines)
{
	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GError) err = NULL;
	guint offset_vals[2];
	gboolean ret;

	line_cfg = gpiodglib_line_config_new();
	settings = gpiodglib_line_settings_new(
			"direction", GPIODGLIB_LINE_DIRECTION_OUTPUT,
			"output-value", GPIODGLIB_LINE_VALUE_ACTIVE,
			NULL);

	offsets = g_array_new(FALSE, TRUE, sizeof(guint));
	offset_vals[0] = 0;
	offset_vals[1] = 2;
	g_array_append_vals(offsets, offset_vals, 2);
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets,
							     settings);
	g_free(g_array_steal(offsets, NULL));

	gpiodglib_line_settings_set_output_value(settings,
						 GPIODGLIB_LINE_VALUE_INACTIVE);
	offset_vals[0] = 1;
	offset_vals[1] = 3;
	g_array_append_vals(offsets, offset_vals, 2);
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets,
							     settings);
	g_free(g_array_steal(offsets, NULL));

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 0), ==,
			G_GPIOSIM_VALUE_ACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 1), ==,
			G_GPIOSIM_VALUE_INACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 2), ==,
			G_GPIOSIM_VALUE_ACTIVE);
	g_assert_cmpint(g_gpiosim_chip_get_value(sim, 3), ==,
			G_GPIOSIM_VALUE_INACTIVE);

	gpiodglib_line_config_reset(line_cfg);

	gpiodglib_line_settings_set_output_value(settings,
						 GPIODGLIB_LINE_VALUE_INACTIVE);
	offset_vals[0] = 0;
	offset_vals[1] = 2;
	g_array_append_vals(offsets, offset_vals, 2);
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets,
							     settings);
	g_free(g_array_steal(offsets, NULL));

	gpiodglib_line_settings_set_output_value(settings,
						 GPIODGLIB_LINE_VALUE_ACTIVE);
	offset_vals[0] = 1;
	offset_vals[1] = 3;
	g_array_append_vals(offsets, offset_vals, 2);
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets,
							     settings);

	ret = gpiodglib_line_request_reconfigure_lines(request, line_cfg, &err);
	g_assert_true(ret);
	g_assert_no_error(err);
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

GPIOD_TEST_CASE(reconfigure_fails_without_config)
{
	static const guint offset = 3;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;

	line_cfg = gpiodglib_line_config_new();
	offsets = gpiodglib_test_array_from_const(&offset, 1, sizeof(guint));

	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets, NULL);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	ret = gpiodglib_line_request_reconfigure_lines(request, NULL, &err);
	g_assert_false(ret);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL);
}

GPIOD_TEST_CASE(reconfigure_with_different_offsets)
{
	static const guint offsets0[] = { 0, 1, 2, 3 };
	static const guint offsets1[] = { 2, 4, 5 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;

	line_cfg = gpiodglib_line_config_new();
	offsets = gpiodglib_test_array_from_const(offsets0, 4, sizeof(guint));
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets, NULL);
	g_free(g_array_steal(offsets, NULL));

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	gpiodglib_line_config_reset(line_cfg);

	g_array_append_vals(offsets, offsets1, 3);
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets, NULL);

	ret = gpiodglib_line_request_reconfigure_lines(request, line_cfg, &err);
	g_assert_false(ret);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL);
}

GPIOD_TEST_CASE(read_one_value)
{
	static const guint offset_vals[] = { 0, 2, 4 };
	static const gint pulls[] = { 0, 1, 0 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GError) err = NULL;
	GpiodglibLineValue value;
	gboolean ret;
	guint i;

	line_cfg = gpiodglib_line_config_new();
	settings = gpiodglib_line_settings_new(
			"direction", GPIODGLIB_LINE_DIRECTION_INPUT, NULL);
	offsets = gpiodglib_test_array_from_const(offset_vals, 3,
						  sizeof(guint));
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets,
							     settings);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	for (i = 0; i < 3; i++)
		g_gpiosim_chip_set_pull(sim, offset_vals[i],
					pulls[i] ? G_GPIOSIM_PULL_UP :
						   G_GPIOSIM_PULL_DOWN);

	ret = gpiodglib_line_request_get_value(request, 2, &value, &err);
	g_assert_true(ret);
	g_assert_no_error(err);
	gpiod_test_return_if_failed();
	g_assert_cmpint(value, ==, GPIODGLIB_LINE_VALUE_ACTIVE);
}

GPIOD_TEST_CASE(read_all_values_null_array)
{
	static const guint offset_vals[] = { 0, 2, 4, 5, 7 };
	static const gint pulls[] = { 0, 1, 0, 1, 1 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GArray) values = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;
	guint i;

	line_cfg = gpiodglib_line_config_new();
	settings = gpiodglib_line_settings_new(
			"direction", GPIODGLIB_LINE_DIRECTION_INPUT, NULL);
	offsets = gpiodglib_test_array_from_const(offset_vals, 5,
						  sizeof(guint));
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets,
							     settings);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	for (i = 0; i < 5; i++)
		g_gpiosim_chip_set_pull(sim, offset_vals[i],
					pulls[i] ? G_GPIOSIM_PULL_UP :
						   G_GPIOSIM_PULL_DOWN);

	ret = gpiodglib_line_request_get_values(request, &values, &err);
	g_assert_true(ret);
	g_assert_no_error(err);
	gpiod_test_return_if_failed();

	g_assert_cmpuint(values->len, ==, 5);

	for (i = 0; i < 5; i++)
		g_assert_cmpint(g_array_index(values, GpiodglibLineValue, i), ==,
				pulls[i]);
}

GPIOD_TEST_CASE(read_all_values_preallocated_array)
{
	static const guint offset_vals[] = { 0, 2, 4, 5, 7 };
	static const gint pulls[] = { 0, 1, 0, 1, 1 };

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GArray) values = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;
	guint i;

	line_cfg = gpiodglib_line_config_new();
	settings = gpiodglib_line_settings_new(
			"direction", GPIODGLIB_LINE_DIRECTION_INPUT, NULL);
	offsets = gpiodglib_test_array_from_const(offset_vals, 5,
						  sizeof(guint));
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets,
							     settings);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	for (i = 0; i < 5; i++)
		g_gpiosim_chip_set_pull(sim, offset_vals[i],
					pulls[i] ? G_GPIOSIM_PULL_UP :
						   G_GPIOSIM_PULL_DOWN);

	values = g_array_new(FALSE, TRUE, sizeof(GpiodglibLineValue));
	g_array_set_size(values, 5);

	ret = gpiodglib_line_request_get_values(request, &values, &err);
	g_assert_true(ret);
	g_assert_no_error(err);
	gpiod_test_return_if_failed();

	g_assert_cmpuint(values->len, ==, 5);

	for (i = 0; i < 5; i++)
		g_assert_cmpint(g_array_index(values, GpiodglibLineValue, i),
				==, pulls[i]);
}

GPIOD_TEST_CASE(set_one_value)
{
	static const guint offset = 4;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;

	line_cfg = gpiodglib_line_config_new();
	settings = gpiodglib_line_settings_new(
			"direction", GPIODGLIB_LINE_DIRECTION_OUTPUT,
			"output-value", GPIODGLIB_LINE_VALUE_INACTIVE,
			NULL);
	offsets = gpiodglib_test_array_from_const(&offset, 1, sizeof(guint));
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets,
							     settings);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	g_assert_cmpuint(g_gpiosim_chip_get_value(sim, offset), ==,
			G_GPIOSIM_VALUE_INACTIVE);

	ret = gpiodglib_line_request_set_value(request, 4,
					       GPIODGLIB_LINE_VALUE_ACTIVE,
					       &err);
	g_assert_true(ret);
	g_assert_no_error(err);

	g_assert_cmpuint(g_gpiosim_chip_get_value(sim, offset), ==,
			 G_GPIOSIM_VALUE_ACTIVE);
}

GPIOD_TEST_CASE(set_all_values)
{
	static const guint offset_vals[] = { 0, 2, 4, 5, 6 };
	static const GpiodglibLineValue value_vals[] = {
		GPIODGLIB_LINE_VALUE_ACTIVE,
		GPIODGLIB_LINE_VALUE_INACTIVE,
		GPIODGLIB_LINE_VALUE_ACTIVE,
		GPIODGLIB_LINE_VALUE_ACTIVE,
		GPIODGLIB_LINE_VALUE_ACTIVE
	};
	static const GPIOSimValue sim_values[] = {
		G_GPIOSIM_VALUE_ACTIVE,
		G_GPIOSIM_VALUE_INACTIVE,
		G_GPIOSIM_VALUE_ACTIVE,
		G_GPIOSIM_VALUE_ACTIVE,
		G_GPIOSIM_VALUE_ACTIVE
	};

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GArray) values = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;
	guint i;

	line_cfg = gpiodglib_line_config_new();
	settings = gpiodglib_line_settings_new(
			"direction", GPIODGLIB_LINE_DIRECTION_OUTPUT, NULL);
	offsets = gpiodglib_test_array_from_const(offset_vals, 5, sizeof(guint));
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets,
							     settings);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	values = gpiodglib_test_array_from_const(value_vals, 5,
						 sizeof(GpiodglibLineValue));

	ret = gpiodglib_line_request_set_values(request, values, &err);
	g_assert_true(ret);
	g_assert_no_error(err);
	gpiod_test_return_if_failed();

	for (i = 0; i < 5; i++)
		g_assert_cmpint(g_gpiosim_chip_get_value(sim, offset_vals[i]),
				==, sim_values[i]);
}

GPIOD_TEST_CASE(get_values_invalid_arguments)
{
	static const guint offset = 3;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GArray) values = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;

	line_cfg = gpiodglib_line_config_new();
	settings = gpiodglib_line_settings_new(
			"direction", GPIODGLIB_LINE_DIRECTION_INPUT, NULL);
	offsets = gpiodglib_test_array_from_const(&offset, 1, sizeof(offset));
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets,
							     settings);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	ret = gpiodglib_line_request_get_values_subset(request, offsets, NULL,
						       &err);
	g_assert_false(ret);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL);

	g_clear_pointer(&err, g_error_free);

	ret = gpiodglib_line_request_get_values_subset(request, NULL, &values,
						       &err);
	g_assert_false(ret);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL);
}

GPIOD_TEST_CASE(set_values_invalid_arguments)
{
	static const guint offset = 3;
	static const GpiodglibLineValue value_vals[] = {
		GPIODGLIB_LINE_VALUE_ACTIVE,
		GPIODGLIB_LINE_VALUE_INACTIVE,
	};

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineSettings) settings = NULL;
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autoptr(GArray) values = NULL;
	g_autoptr(GArray) vals_inval = NULL;
	g_autoptr(GError) err = NULL;
	gboolean ret;

	line_cfg = gpiodglib_line_config_new();
	settings = gpiodglib_line_settings_new(
			"direction", GPIODGLIB_LINE_DIRECTION_OUTPUT, NULL);
	offsets = gpiodglib_test_array_from_const(&offset, 1, sizeof(offset));
	values = gpiodglib_test_array_from_const(value_vals, 1,
						 sizeof(GpiodglibLineValue));
	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets,
							     settings);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	ret = gpiodglib_line_request_set_values_subset(request, offsets, NULL,
						       &err);
	g_assert_false(ret);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL);

	g_clear_pointer(&err, g_error_free);

	ret = gpiodglib_line_request_set_values_subset(request, NULL, values,
						       &err);
	g_assert_false(ret);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL);

	g_clear_pointer(&err, g_error_free);

	vals_inval = gpiodglib_test_array_from_const(value_vals, 2,
						sizeof(GpiodglibLineValue));

	ret = gpiodglib_line_request_set_values_subset(request, offsets,
						       vals_inval, &err);
	g_assert_false(ret);
	g_assert_error(err, GPIODGLIB_ERROR, GPIODGLIB_ERR_INVAL);
}

GPIOD_TEST_CASE(get_chip_name)
{
	static const guint offset = 4;

	g_autoptr(GPIOSimChip) sim = g_gpiosim_chip_new("num-lines", 8, NULL);
	g_autoptr(GpiodglibLineConfig) line_cfg = NULL;
	g_autoptr(GpiodglibLineRequest) request = NULL;
	g_autoptr(GArray) offsets = NULL;
	g_autofree gchar *name = NULL;

	line_cfg = gpiodglib_line_config_new();
	offsets = gpiodglib_test_array_from_const(&offset, 1, sizeof(guint));

	gpiodglib_test_line_config_add_line_settings_or_fail(line_cfg,
							     offsets, NULL);

	request = gpiodglib_test_request_lines_or_fail(
			g_gpiosim_chip_get_dev_path(sim), NULL, line_cfg);

	name = gpiodglib_line_request_dup_chip_name(request);
	g_assert_cmpstr(g_gpiosim_chip_get_name(sim), ==, name);
}
