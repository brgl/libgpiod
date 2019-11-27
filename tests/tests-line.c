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
	g_assert_cmpstr(gpiod_line_consumer(line), ==,
			"consumer string over 32 charact");
	g_assert_cmpuint(strlen(gpiod_line_consumer(line)), ==, 31);
}

GPIOD_TEST_CASE(request_bulk_output, 0, { 8, 8 })
{
	g_autoptr(gpiod_chip_struct) chipA = NULL;
	g_autoptr(gpiod_chip_struct) chipB = NULL;
	struct gpiod_line_bulk bulkB = GPIOD_LINE_BULK_INITIALIZER, bulkA;
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

	gpiod_line_bulk_init(&bulkA);
	gpiod_line_bulk_add(&bulkA, lineA0);
	gpiod_line_bulk_add(&bulkA, lineA1);
	gpiod_line_bulk_add(&bulkA, lineA2);
	gpiod_line_bulk_add(&bulkA, lineA3);
	gpiod_line_bulk_add(&bulkB, lineB0);
	gpiod_line_bulk_add(&bulkB, lineB1);
	gpiod_line_bulk_add(&bulkB, lineB2);
	gpiod_line_bulk_add(&bulkB, lineB3);

	valA[0] = 1;
	valA[1] = 0;
	valA[2] = 0;
	valA[3] = 1;
	ret = gpiod_line_request_bulk_output(&bulkA,
					     GPIOD_TEST_CONSUMER, valA);
	g_assert_cmpint(ret, ==, 0);

	valB[0] = 0;
	valB[1] = 1;
	valB[2] = 0;
	valB[3] = 1;
	ret = gpiod_line_request_bulk_output(&bulkB,
					     GPIOD_TEST_CONSUMER, valB);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 3), ==, 1);

	g_assert_cmpint(gpiod_test_chip_get_value(1, 0), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(1, 1), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(1, 2), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(1, 3), ==, 1);
}

GPIOD_TEST_CASE(request_bulk_different_chips, 0, { 8, 8 })
{
	g_autoptr(gpiod_chip_struct) chipA = NULL;
	g_autoptr(gpiod_chip_struct) chipB = NULL;
	struct gpiod_line *lineA0, *lineA1, *lineB0, *lineB1;
	struct gpiod_line_request_config req;
	struct gpiod_line_bulk bulk;
	gint ret;

	chipA = gpiod_chip_open(gpiod_test_chip_path(0));
	chipB = gpiod_chip_open(gpiod_test_chip_path(1));
	g_assert_nonnull(chipA);
	g_assert_nonnull(chipB);
	gpiod_test_return_if_failed();

	lineA0 = gpiod_chip_get_line(chipA, 0);
	lineA1 = gpiod_chip_get_line(chipA, 1);
	lineB0 = gpiod_chip_get_line(chipB, 0);
	lineB1 = gpiod_chip_get_line(chipB, 1);

	g_assert_nonnull(lineA0);
	g_assert_nonnull(lineA1);
	g_assert_nonnull(lineB0);
	g_assert_nonnull(lineB1);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_init(&bulk);
	gpiod_line_bulk_add(&bulk, lineA0);
	gpiod_line_bulk_add(&bulk, lineA1);
	gpiod_line_bulk_add(&bulk, lineB0);
	gpiod_line_bulk_add(&bulk, lineB1);

	req.consumer = GPIOD_TEST_CONSUMER;
	req.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
	req.flags = GPIOD_LINE_ACTIVE_STATE_HIGH;

	ret = gpiod_line_request_bulk(&bulk, &req, NULL);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);
}

GPIOD_TEST_CASE(request_null_default_vals_for_output, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_bulk bulk = GPIOD_LINE_BULK_INITIALIZER;
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

	gpiod_line_bulk_add(&bulk, line0);
	gpiod_line_bulk_add(&bulk, line1);
	gpiod_line_bulk_add(&bulk, line2);

	ret = gpiod_line_request_bulk_output(&bulk, GPIOD_TEST_CONSUMER, NULL);
	g_assert_cmpint(ret, ==, 0);

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

	ret = gpiod_line_set_value(line, 1);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);
	ret = gpiod_line_set_value(line, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);
}

GPIOD_TEST_CASE(set_value_bulk, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_bulk bulk = GPIOD_LINE_BULK_INITIALIZER;
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

	gpiod_line_bulk_add(&bulk, line0);
	gpiod_line_bulk_add(&bulk, line1);
	gpiod_line_bulk_add(&bulk, line2);

	values[0] = 0;
	values[1] = 1;
	values[2] = 2;

	ret = gpiod_line_request_bulk_output(&bulk,
			GPIOD_TEST_CONSUMER, values);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 1);

	values[0] = 2;
	values[1] = 1;
	values[2] = 0;

	ret = gpiod_line_set_value_bulk(&bulk, values);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 1);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);

	ret = gpiod_line_set_value_bulk(&bulk, NULL);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 0), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 1), ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 2), ==, 0);
}

GPIOD_TEST_CASE(get_value_different_chips, 0, { 8, 8 })
{
	g_autoptr(gpiod_chip_struct) chipA = NULL;
	g_autoptr(gpiod_chip_struct) chipB = NULL;
	struct gpiod_line *lineA0, *lineA1, *lineB0, *lineB1;
	struct gpiod_line_bulk bulkA, bulkB, bulk;
	gint ret, vals[4];

	chipA = gpiod_chip_open(gpiod_test_chip_path(0));
	chipB = gpiod_chip_open(gpiod_test_chip_path(1));
	g_assert_nonnull(chipA);
	g_assert_nonnull(chipB);
	gpiod_test_return_if_failed();

	lineA0 = gpiod_chip_get_line(chipA, 3);
	lineA1 = gpiod_chip_get_line(chipA, 4);
	lineB0 = gpiod_chip_get_line(chipB, 5);
	lineB1 = gpiod_chip_get_line(chipB, 6);

	g_assert_nonnull(lineA0);
	g_assert_nonnull(lineA1);
	g_assert_nonnull(lineB0);
	g_assert_nonnull(lineB1);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_init(&bulkA);
	gpiod_line_bulk_init(&bulkB);
	gpiod_line_bulk_init(&bulk);

	gpiod_line_bulk_add(&bulkA, lineA0);
	gpiod_line_bulk_add(&bulkA, lineA1);
	gpiod_line_bulk_add(&bulkB, lineB0);
	gpiod_line_bulk_add(&bulkB, lineB1);
	gpiod_line_bulk_add(&bulk, lineA0);
	gpiod_line_bulk_add(&bulk, lineA1);
	gpiod_line_bulk_add(&bulk, lineB0);
	gpiod_line_bulk_add(&bulk, lineB1);

	ret = gpiod_line_request_bulk_input(&bulkA, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	ret = gpiod_line_request_bulk_input(&bulkB, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);

	ret = gpiod_line_get_value_bulk(&bulk, vals);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);
}

GPIOD_TEST_CASE(get_line_helper, 0, { 16, 16, 32, 16 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;

	line = gpiod_line_get(gpiod_test_chip_name(2), 18);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();
	chip = gpiod_line_get_chip(line);
	g_assert_cmpint(gpiod_line_offset(line), ==, 18);
}

GPIOD_TEST_CASE(get_line_helper_invalid_offset, 0, { 16, 16, 32, 16 })
{
	struct gpiod_line *line;

	line = gpiod_line_get(gpiod_test_chip_name(3), 18);
	g_assert_null(line);
	g_assert_cmpint(errno, ==, EINVAL);
}

GPIOD_TEST_CASE(find_good, GPIOD_TEST_FLAG_NAMED_LINES, { 16, 16, 32, 16 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;

	line = gpiod_line_find("gpio-mockup-C-12");
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();
	chip = gpiod_line_get_chip(line);
	g_assert_cmpint(gpiod_line_offset(line), ==, 12);
}

GPIOD_TEST_CASE(find_not_found,
		GPIOD_TEST_FLAG_NAMED_LINES, { 16, 16, 32, 16 })
{
	struct gpiod_line *line;

	line = gpiod_line_find("nonexistent");
	g_assert_null(line);
	g_assert_cmpint(errno, ==, ENOENT);
}

GPIOD_TEST_CASE(find_unnamed_lines, 0, { 16, 16, 32, 16 })
{
	struct gpiod_line *line;

	line = gpiod_line_find("gpio-mockup-C-12");
	g_assert_null(line);
	g_assert_cmpint(errno, ==, ENOENT);
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

	ret = gpiod_line_request_output(line, GPIOD_TEST_CONSUMER, 0);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);

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

	g_assert_cmpint(gpiod_line_active_state(line), ==,
			GPIOD_LINE_ACTIVE_STATE_HIGH);

	gpiod_line_release(line);

	ret = gpiod_line_request_input_flags(line, GPIOD_TEST_CONSUMER,
					GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_INPUT);
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
	g_assert_false(gpiod_line_is_open_drain(line));
	g_assert_false(gpiod_line_is_open_source(line));

	config.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
	config.consumer = GPIOD_TEST_CONSUMER;
	config.flags = GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);

	g_assert_true(gpiod_line_is_used(line));
	g_assert_true(gpiod_line_is_open_drain(line));
	g_assert_false(gpiod_line_is_open_source(line));
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);

	gpiod_line_release(line);

	config.flags = GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);

	g_assert_true(gpiod_line_is_used(line));
	g_assert_false(gpiod_line_is_open_drain(line));
	g_assert_true(gpiod_line_is_open_source(line));
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);
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

	g_assert_true(gpiod_line_is_used(line));
	g_assert_true(gpiod_line_is_open_drain(line));
	g_assert_false(gpiod_line_is_open_source(line));
	g_assert_cmpint(gpiod_line_active_state(line), ==,
			GPIOD_LINE_ACTIVE_STATE_LOW);
	g_assert_cmpint(gpiod_line_direction(line), ==,
			GPIOD_LINE_DIRECTION_OUTPUT);

	gpiod_line_release(line);

	config.flags = GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE |
		       GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;

	ret = gpiod_line_request(line, &config, 0);
	g_assert_cmpint(ret, ==, 0);

	g_assert_true(gpiod_line_is_used(line));
	g_assert_false(gpiod_line_is_open_drain(line));
	g_assert_true(gpiod_line_is_open_source(line));
	g_assert_cmpint(gpiod_line_active_state(line), ==,
			GPIOD_LINE_ACTIVE_STATE_LOW);
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

/* Verify that the reference counting of the line fd handle works correctly. */
GPIOD_TEST_CASE(release_one_use_another, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_bulk bulk;
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

	gpiod_line_bulk_init(&bulk);
	gpiod_line_bulk_add(&bulk, line0);
	gpiod_line_bulk_add(&bulk, line1);

	vals[0] = vals[1] = 1;

	ret = gpiod_line_request_bulk_output(&bulk, GPIOD_TEST_CONSUMER, vals);
	g_assert_cmpint(ret, ==, 0);

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

GPIOD_TEST_CASE(bulk_foreach, GPIOD_TEST_FLAG_NAMED_LINES, { 8 })
{
	static const gchar *const line_names[] = { "gpio-mockup-A-0",
						   "gpio-mockup-A-1",
						   "gpio-mockup-A-2",
						   "gpio-mockup-A-3" };

	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_bulk bulk = GPIOD_LINE_BULK_INITIALIZER;
	struct gpiod_line *line, **lineptr;
	gint i;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	for (i = 0; i < 4; i++) {
		line = gpiod_chip_get_line(chip, i);
		g_assert_nonnull(line);
		gpiod_test_return_if_failed();

		gpiod_line_bulk_add(&bulk, line);
	}

	i = 0;
	gpiod_line_bulk_foreach_line(&bulk, line, lineptr)
		g_assert_cmpstr(gpiod_line_name(line), ==, line_names[i++]);

	i = 0;
	gpiod_line_bulk_foreach_line(&bulk, line, lineptr) {
		g_assert_cmpstr(gpiod_line_name(line), ==, line_names[i++]);
		if (i == 2)
			break;
	}
}
