// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>
#include <string.h>

#include "gpiod-test.h"

#define GPIOD_TEST_GROUP "line"

GPIOD_TEST_CASE(request_output, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line0;
	struct gpiod_line *line1;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line0 = gpiod_chip_get_line(chip, 2);
	line1 = gpiod_chip_get_line(chip, 5);
	g_assert_nonnull(line0);
	g_assert_nonnull(line1);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_output(line0, GPIOD_TEST_CONSUMER, 0);
	g_assert_cmpint(ret, ==, 0);
	ret = gpiod_line_request_output(line1, GPIOD_TEST_CONSUMER, 1);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 5), ==, 1);
}

GPIOD_TEST_CASE(request_already_requested, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 0);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EBUSY);
}

GPIOD_TEST_CASE(consumer, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 0);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	g_assert_null(gpiod_line_consumer(line));

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpstr(gpiod_line_consumer(line), ==, GPIOD_TEST_CONSUMER);
}

GPIOD_TEST_CASE(consumer_long_string, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 0);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	g_assert_null(gpiod_line_consumer(line));

	ret = gpiod_line_request_input(line,
			"consumer string over 32 characters long");
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_cmpstr(gpiod_line_consumer(line), ==,
			"consumer string over 32 charact");
	g_assert_cmpuint(strlen(gpiod_line_consumer(line)), ==, 31);
}

GPIOD_TEST_CASE(request_bulk_output, 0, { 8, 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulkA = NULL;
	g_autoptr(gpiod_line_bulk_struct) bulkB = NULL;
	g_autoptr(gpiod_chip_struct) chipA = NULL;
	g_autoptr(gpiod_chip_struct) chipB = NULL;
	struct gpiod_line *lineA0, *lineA1, *lineA2, *lineA3,
			  *lineB0, *lineB1, *lineB2, *lineB3;
	gint valA[4], valB[4], ret;

	chipA = gpiod_chip_open(gpiod_test_chip_path(0));
	chipB = gpiod_chip_open(gpiod_test_chip_path(1));
	g_assert_nonnull(chipA);
	g_assert_nonnull(chipB);
	gpiod_test_return_if_failed();

	lineA0 = gpiod_chip_get_line(chipA, 0);
	lineA1 = gpiod_chip_get_line(chipA, 1);
	lineA2 = gpiod_chip_get_line(chipA, 2);
	lineA3 = gpiod_chip_get_line(chipA, 3);
	lineB0 = gpiod_chip_get_line(chipB, 0);
	lineB1 = gpiod_chip_get_line(chipB, 1);
	lineB2 = gpiod_chip_get_line(chipB, 2);
	lineB3 = gpiod_chip_get_line(chipB, 3);

	g_assert_nonnull(lineA0);
	g_assert_nonnull(lineA1);
	g_assert_nonnull(lineA2);
	g_assert_nonnull(lineA3);
	g_assert_nonnull(lineB0);
	g_assert_nonnull(lineB1);
	g_assert_nonnull(lineB2);
	g_assert_nonnull(lineB3);
	gpiod_test_return_if_failed();

	bulkA = gpiod_line_bulk_new(4);
	bulkB = gpiod_line_bulk_new(4);
	g_assert_nonnull(bulkA);
	g_assert_nonnull(bulkB);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_add_line(bulkA, lineA0);
	gpiod_line_bulk_add_line(bulkA, lineA1);
	gpiod_line_bulk_add_line(bulkA, lineA2);
	gpiod_line_bulk_add_line(bulkA, lineA3);
	gpiod_line_bulk_add_line(bulkB, lineB0);
	gpiod_line_bulk_add_line(bulkB, lineB1);
	gpiod_line_bulk_add_line(bulkB, lineB2);
	gpiod_line_bulk_add_line(bulkB, lineB3);

	valA[0] = 1;
	valA[1] = 0;
	valA[2] = 0;
	valA[3] = 1;
	ret = gpiod_line_request_bulk_output(bulkA,
					     GPIOD_TEST_CONSUMER, valA);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	valB[0] = 0;
	valB[1] = 1;
	valB[2] = 0;
	valB[3] = 1;
	ret = gpiod_line_request_bulk_output(bulkB,
					     GPIOD_TEST_CONSUMER, valB);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 3), ==, 1);

	g_assert_cmpint(gpiod_test_chip_get_value(1, 0), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(1, 1), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(1, 2), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(1, 3), ==, 1);
}

GPIOD_TEST_CASE(request_null_default_vals_for_output, 0, { 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line0, *line1, *line2;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line0 = gpiod_chip_get_line(chip, 0);
	line1 = gpiod_chip_get_line(chip, 1);
	line2 = gpiod_chip_get_line(chip, 2);

	g_assert_nonnull(line0);
	g_assert_nonnull(line1);
	g_assert_nonnull(line2);
	gpiod_test_return_if_failed();

	bulk = gpiod_line_bulk_new(3);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_add_line(bulk, line0);
	gpiod_line_bulk_add_line(bulk, line1);
	gpiod_line_bulk_add_line(bulk, line2);

	ret = gpiod_line_request_bulk_output(bulk, GPIOD_TEST_CONSUMER, NULL);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);
}

GPIOD_TEST_CASE(set_value, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_output(line, GPIOD_TEST_CONSUMER, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);

	ret = gpiod_line_set_value(line, 1);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);
	ret = gpiod_line_set_value(line, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);
}

GPIOD_TEST_CASE(set_value_bulk, 0, { 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line0, *line1, *line2;
	int values[3];
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line0 = gpiod_chip_get_line(chip, 0);
	line1 = gpiod_chip_get_line(chip, 1);
	line2 = gpiod_chip_get_line(chip, 2);

	g_assert_nonnull(line0);
	g_assert_nonnull(line1);
	g_assert_nonnull(line2);
	gpiod_test_return_if_failed();

	bulk = gpiod_line_bulk_new(3);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_add_line(bulk, line0);
	gpiod_line_bulk_add_line(bulk, line1);
	gpiod_line_bulk_add_line(bulk, line2);

	values[0] = 0;
	values[1] = 1;
	values[2] = 2;

	ret = gpiod_line_request_bulk_output(bulk,
			GPIOD_TEST_CONSUMER, values);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);

	values[0] = 2;
	values[1] = 1;
	values[2] = 0;

	ret = gpiod_line_set_value_bulk(bulk, values);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);

	ret = gpiod_line_set_value_bulk(bulk, NULL);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);
}

GPIOD_TEST_CASE(set_config_bulk_null_values, 0, { 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line0, *line1, *line2;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line0 = gpiod_chip_get_line(chip, 0);
	line1 = gpiod_chip_get_line(chip, 1);
	line2 = gpiod_chip_get_line(chip, 2);

	g_assert_nonnull(line0);
	g_assert_nonnull(line1);
	g_assert_nonnull(line2);
	gpiod_test_return_if_failed();

	bulk = gpiod_line_bulk_new(3);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_add_line(bulk, line0);
	gpiod_line_bulk_add_line(bulk, line1);
	gpiod_line_bulk_add_line(bulk, line2);

	ret = gpiod_line_request_bulk_output(bulk, GPIOD_TEST_CONSUMER, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_false(gpiod_line_is_active_low(line0));
	g_assert_false(gpiod_line_is_active_low(line1));
	g_assert_false(gpiod_line_is_active_low(line2));

	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);

	ret = gpiod_line_set_config_bulk(bulk,
			GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
			GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW, NULL);
	g_assert_cmpint(ret, ==, 0);
	g_assert_true(gpiod_line_is_active_low(line0));
	g_assert_true(gpiod_line_is_active_low(line1));
	g_assert_true(gpiod_line_is_active_low(line2));
	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);

	ret = gpiod_line_set_config_bulk(bulk,
			GPIOD_LINE_REQUEST_DIRECTION_OUTPUT, 0, NULL);
	g_assert_cmpint(ret, ==, 0);
	g_assert_false(gpiod_line_is_active_low(line0));
	g_assert_false(gpiod_line_is_active_low(line1));
	g_assert_false(gpiod_line_is_active_low(line2));
	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);
}

GPIOD_TEST_CASE(set_flags_active_state, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_output(line, GPIOD_TEST_CONSUMER, 1);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_false(gpiod_line_is_active_low(line));
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);

	ret = gpiod_line_set_flags(line, GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW);
	g_assert_cmpint(ret, ==, 0);
	g_assert_true(gpiod_line_is_active_low(line));
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);

	ret = gpiod_line_set_flags(line, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_false(gpiod_line_is_active_low(line));
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);
}

GPIOD_TEST_CASE(set_flags_bias, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_cmpint(gpiod_line_bias(line), ==, GPIOD_LINE_BIAS_UNKNOWN);

	ret = gpiod_line_set_flags(line,
		GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLED);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_bias(line), ==, GPIOD_LINE_BIAS_DISABLED);

	ret = gpiod_line_set_flags(line,
		GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_bias(line), ==, GPIOD_LINE_BIAS_PULL_UP);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);

	ret = gpiod_line_set_flags(line,
		GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_bias(line), ==, GPIOD_LINE_BIAS_PULL_DOWN);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);
}

GPIOD_TEST_CASE(set_flags_drive, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_output(line, GPIOD_TEST_CONSUMER, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_cmpint(gpiod_line_drive(line), ==, GPIOD_LINE_DRIVE_PUSH_PULL);

	ret = gpiod_line_set_flags(line,
		GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_drive(line), ==,
			GPIOD_LINE_DRIVE_OPEN_DRAIN);

	ret = gpiod_line_set_flags(line,
		GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_drive(line), ==,
			GPIOD_LINE_DRIVE_OPEN_SOURCE);
}

GPIOD_TEST_CASE(set_direction, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_output(line, GPIOD_TEST_CONSUMER, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);

	ret = gpiod_line_set_direction_input(line);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_INPUT);

	ret = gpiod_line_set_direction_output(line, 1);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);
}

GPIOD_TEST_CASE(set_direction_bulk, 0, { 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line0, *line1, *line2;
	int values[3];
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line0 = gpiod_chip_get_line(chip, 0);
	line1 = gpiod_chip_get_line(chip, 1);
	line2 = gpiod_chip_get_line(chip, 2);

	g_assert_nonnull(line0);
	g_assert_nonnull(line1);
	g_assert_nonnull(line2);
	gpiod_test_return_if_failed();

	bulk = gpiod_line_bulk_new(3);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_add_line(bulk, line0);
	gpiod_line_bulk_add_line(bulk, line1);
	gpiod_line_bulk_add_line(bulk, line2);

	values[0] = 0;
	values[1] = 1;
	values[2] = 2;

	ret = gpiod_line_request_bulk_output(bulk,
			GPIOD_TEST_CONSUMER, values);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_cmpint(gpiod_line_direction(line0), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_line_direction(line1), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_line_direction(line2), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);

	ret = gpiod_line_set_direction_input_bulk(bulk);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_direction(line0), ==,
			GPIOD_LINE_DIRECTION_INPUT);
	g_assert_cmpint(gpiod_line_direction(line1), ==,
			GPIOD_LINE_DIRECTION_INPUT);
	g_assert_cmpint(gpiod_line_direction(line2), ==,
			GPIOD_LINE_DIRECTION_INPUT);

	values[0] = 2;
	values[1] = 1;
	values[2] = 0;

	ret = gpiod_line_set_direction_output_bulk(bulk, values);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_direction(line0), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_line_direction(line1), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_line_direction(line2), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);

	ret = gpiod_line_set_direction_output_bulk(bulk, NULL);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_direction(line0), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_line_direction(line1), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_line_direction(line2), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);
}

GPIOD_TEST_CASE(output_value_caching, 0, { 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	/* check cached by request... */
	ret = gpiod_line_request_output(line, GPIOD_TEST_CONSUMER, 1);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);

	/* ...by checking cached value applied by set_flags */
	ret = gpiod_line_set_flags(line, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);

	/* check cached by set_value */
	ret = gpiod_line_set_value(line, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);

	ret = gpiod_line_set_flags(line, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);

	ret = gpiod_line_set_value(line, 1);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);

	ret = gpiod_line_set_flags(line, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);

	/* check cached by set_config */
	ret = gpiod_line_set_config(line, GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
				    0, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);

	ret = gpiod_line_set_flags(line, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);

	/* check cached by set_value_bulk default */
	ret = gpiod_line_set_value(line, 1);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);

	bulk = gpiod_line_bulk_new(1);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_add_line(bulk, line);
	ret = gpiod_line_set_value_bulk(bulk, NULL);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);

	ret = gpiod_line_set_flags(line, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);
}

GPIOD_TEST_CASE(direction, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 5);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_output(line, GPIOD_TEST_CONSUMER, 1);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 5), ==, 1);

	gpiod_line_release(line);

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_INPUT);
}

GPIOD_TEST_CASE(active_state, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 5);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_false(gpiod_line_is_active_low(line));

	gpiod_line_release(line);

	ret = gpiod_line_request_input_flags(line, GPIOD_TEST_CONSUMER,
					GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_INPUT);

	gpiod_line_release(line);

	ret = gpiod_line_request_output_flags(line, GPIOD_TEST_CONSUMER,
			GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 5), ==, 1);

	gpiod_line_release(line);

	ret = gpiod_line_request_output_flags(line,
			GPIOD_TEST_CONSUMER, 0, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 5), ==, 0);

}

GPIOD_TEST_CASE(misc_flags, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_request_config config;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	g_assert_false(gpiod_line_is_used(line));
	g_assert_cmpint(gpiod_line_drive(line), ==, GPIOD_LINE_DRIVE_PUSH_PULL);
	g_assert_cmpint(gpiod_line_bias(line), ==, GPIOD_LINE_BIAS_UNKNOWN);

	config.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
	config.consumer = GPIOD_TEST_CONSUMER;
	config.flags = GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_true(gpiod_line_is_used(line));
	g_assert_cmpint(gpiod_line_drive(line), ==,
			GPIOD_LINE_DRIVE_OPEN_DRAIN);
	g_assert_cmpint(gpiod_line_bias(line), ==, GPIOD_LINE_BIAS_UNKNOWN);
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);

	gpiod_line_release(line);

	config.flags = GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_true(gpiod_line_is_used(line));
	g_assert_cmpint(gpiod_line_drive(line), ==,
			GPIOD_LINE_DRIVE_OPEN_SOURCE);
	g_assert_cmpint(gpiod_line_bias(line), ==, GPIOD_LINE_BIAS_UNKNOWN);
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);

	gpiod_line_release(line);
}

GPIOD_TEST_CASE(misc_flags_work_together, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_request_config config;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	/*
	 * Verify that open drain/source flags work together
	 * with active_low.
	 */

	config.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
	config.consumer = GPIOD_TEST_CONSUMER;
	config.flags = GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN |
		       GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_true(gpiod_line_is_used(line));
	g_assert_cmpint(gpiod_line_drive(line), ==,
			GPIOD_LINE_DRIVE_OPEN_DRAIN);
	g_assert_cmpint(gpiod_line_bias(line), ==, GPIOD_LINE_BIAS_UNKNOWN);
	g_assert_true(gpiod_line_is_active_low(line));
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);

	gpiod_line_release(line);

	config.flags = GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE |
		       GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_true(gpiod_line_is_used(line));
	g_assert_cmpint(gpiod_line_drive(line), ==,
			GPIOD_LINE_DRIVE_OPEN_SOURCE);
	g_assert_cmpint(gpiod_line_bias(line), ==, GPIOD_LINE_BIAS_UNKNOWN);
	g_assert_true(gpiod_line_is_active_low(line));

	gpiod_line_release(line);

	/*
	 * Verify that pull-up/down flags work together
	 * with active_low.
	 */

	config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
	config.flags = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN |
		       GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_true(gpiod_line_is_used(line));
	g_assert_cmpint(gpiod_line_drive(line), ==, GPIOD_LINE_DRIVE_PUSH_PULL);
	g_assert_cmpint(gpiod_line_bias(line), ==, GPIOD_LINE_BIAS_PULL_DOWN);
	g_assert_true(gpiod_line_is_active_low(line));
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_INPUT);

	ret = gpiod_line_get_value(line);
	g_assert_cmpint(ret, ==, 1);

	gpiod_line_release(line);

	config.flags = GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP |
		       GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	g_assert_true(gpiod_line_is_used(line));
	g_assert_cmpint(gpiod_line_drive(line), ==, GPIOD_LINE_DRIVE_PUSH_PULL);
	g_assert_cmpint(gpiod_line_bias(line), ==, GPIOD_LINE_BIAS_PULL_UP);
	g_assert_true(gpiod_line_is_active_low(line));
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_INPUT);

	ret = gpiod_line_get_value(line);
	g_assert_cmpint(ret, ==, 0);

	gpiod_line_release(line);
}

GPIOD_TEST_CASE(open_source_open_drain_input_mode, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_input_flags(line, GPIOD_TEST_CONSUMER,
					GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);

	ret = gpiod_line_request_input_flags(line, GPIOD_TEST_CONSUMER,
					GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);
}

GPIOD_TEST_CASE(open_source_open_drain_simultaneously, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_output_flags(line, GPIOD_TEST_CONSUMER,
					GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE |
					GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN, 1);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);
}

GPIOD_TEST_CASE(multiple_bias_flags, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_input_flags(line, GPIOD_TEST_CONSUMER,
					GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLED |
					GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);

	ret = gpiod_line_request_input_flags(line, GPIOD_TEST_CONSUMER,
					GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLED |
					GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);

	ret = gpiod_line_request_input_flags(line, GPIOD_TEST_CONSUMER,
					GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN |
					GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);

	ret = gpiod_line_request_input_flags(line, GPIOD_TEST_CONSUMER,
					GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLED |
					GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN |
					GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);
}


/* Verify that the reference counting of the line fd handle works correctly. */
GPIOD_TEST_CASE(release_one_use_another, 0, { 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line0;
	struct gpiod_line *line1;
	gint ret, vals[2];

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line0 = gpiod_chip_get_line(chip, 2);
	line1 = gpiod_chip_get_line(chip, 3);
	g_assert_nonnull(line0);
	g_assert_nonnull(line1);
	gpiod_test_return_if_failed();

	bulk = gpiod_line_bulk_new(2);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_add_line(bulk, line0);
	gpiod_line_bulk_add_line(bulk, line1);

	vals[0] = vals[1] = 1;

	ret = gpiod_line_request_bulk_output(bulk, GPIOD_TEST_CONSUMER, vals);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	gpiod_line_release(line0);

	ret = gpiod_line_get_value(line0);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EPERM);
}

GPIOD_TEST_CASE(null_consumer, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_request_config config;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
	config.consumer = NULL;
	config.flags = 0;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_cmpstr(gpiod_line_consumer(line), ==, "?");

	gpiod_line_release(line);

	/*
	 * Internally we use different structures for event requests, so we
	 * need to test that explicitly too.
	 */
	config.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpstr(gpiod_line_consumer(line), ==, "?");
}

GPIOD_TEST_CASE(empty_consumer, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_request_config config;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 2);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
	config.consumer = "";
	config.flags = 0;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();
	g_assert_cmpstr(gpiod_line_consumer(line), ==, "?");

	gpiod_line_release(line);

	/*
	 * Internally we use different structures for event requests, so we
	 * need to test that explicitly too.
	 */
	config.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpstr(gpiod_line_consumer(line), ==, "?");
}
