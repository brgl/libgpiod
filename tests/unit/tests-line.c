/*
 * GPIO line test cases for libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "gpiod-unit.h"

static void line_request_output(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;
	struct gpiod_line *line_0;
	struct gpiod_line *line_1;
	int status;

	chip = gpiod_chip_open(gu_chip_path(0));
	GU_ASSERT_NOT_NULL(chip);

	line_0 = gpiod_chip_get_line(chip, 2);
	line_1 = gpiod_chip_get_line(chip, 5);
	GU_ASSERT_NOT_NULL(line_0);
	GU_ASSERT_NOT_NULL(line_1);

	status = gpiod_line_request_output(line_0, "gpiod-unit", false, 0);
	GU_ASSERT_RET_OK(status);
	status = gpiod_line_request_output(line_1, "gpiod-unit", false, 1);
	GU_ASSERT_RET_OK(status);

	GU_ASSERT_EQ(gpiod_line_get_value(line_0), 0);
	GU_ASSERT_EQ(gpiod_line_get_value(line_1), 1);

	gpiod_line_release(line_0);
	gpiod_line_release(line_1);
}
GU_DEFINE_TEST(line_request_output,
	       "gpiod_line_request_output() - good",
	       GU_LINES_UNNAMED, { 8 });

static void line_request_already_requested(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;
	struct gpiod_line *line;
	int status;

	chip = gpiod_chip_open(gu_chip_path(0));
	GU_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 0);
	GU_ASSERT_NOT_NULL(line);

	status = gpiod_line_request_input(line, "gpiod-unit", false);
	GU_ASSERT_RET_OK(status);

	status = gpiod_line_request_input(line, "gpiod-unit", false);
	GU_ASSERT_NOTEQ(status, 0);
	GU_ASSERT_EQ(gpiod_errno(), GPIOD_ELINEBUSY);
}
GU_DEFINE_TEST(line_request_already_requested,
	       "gpiod_line_request() - already requested",
	       GU_LINES_UNNAMED, { 8 });

static void line_consumer(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;
	struct gpiod_line *line;
	int status;

	chip = gpiod_chip_open(gu_chip_path(0));
	GU_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 0);
	GU_ASSERT_NOT_NULL(line);

	GU_ASSERT_NULL(gpiod_line_consumer(line));

	status = gpiod_line_request_input(line, "gpiod-unit", false);
	GU_ASSERT_RET_OK(status);

	GU_ASSERT(!gpiod_line_needs_update(line));
	GU_ASSERT_STR_EQ(gpiod_line_consumer(line), "gpiod-unit");
}
GU_DEFINE_TEST(line_consumer,
	       "gpiod_line_consumer() - good",
	       GU_LINES_UNNAMED, { 8 });

static void line_request_bulk_output(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chipA = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chipB = NULL;
	struct gpiod_line *lineA0;
	struct gpiod_line *lineA1;
	struct gpiod_line *lineA2;
	struct gpiod_line *lineA3;
	struct gpiod_line *lineB0;
	struct gpiod_line *lineB1;
	struct gpiod_line *lineB2;
	struct gpiod_line *lineB3;
	struct gpiod_line_bulk bulkA;
	struct gpiod_line_bulk bulkB = GPIOD_LINE_BULK_INITIALIZER;
	int status;
	int valA[4], valB[4];

	chipA = gpiod_chip_open(gu_chip_path(0));
	chipB = gpiod_chip_open(gu_chip_path(1));
	GU_ASSERT_NOT_NULL(chipA);
	GU_ASSERT_NOT_NULL(chipB);

	gpiod_line_bulk_init(&bulkA);

	lineA0 = gpiod_chip_get_line(chipA, 0);
	lineA1 = gpiod_chip_get_line(chipA, 1);
	lineA2 = gpiod_chip_get_line(chipA, 2);
	lineA3 = gpiod_chip_get_line(chipA, 3);
	lineB0 = gpiod_chip_get_line(chipB, 0);
	lineB1 = gpiod_chip_get_line(chipB, 1);
	lineB2 = gpiod_chip_get_line(chipB, 2);
	lineB3 = gpiod_chip_get_line(chipB, 3);

	GU_ASSERT_NOT_NULL(lineA0);
	GU_ASSERT_NOT_NULL(lineA1);
	GU_ASSERT_NOT_NULL(lineA2);
	GU_ASSERT_NOT_NULL(lineA3);
	GU_ASSERT_NOT_NULL(lineB0);
	GU_ASSERT_NOT_NULL(lineB1);
	GU_ASSERT_NOT_NULL(lineB2);
	GU_ASSERT_NOT_NULL(lineB3);

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
	status = gpiod_line_request_bulk_output(&bulkA, "gpiod-unit",
						false, valA);
	GU_ASSERT_RET_OK(status);

	valB[0] = 0;
	valB[1] = 1;
	valB[2] = 0;
	valB[3] = 1;
	status = gpiod_line_request_bulk_output(&bulkB, "gpiod-unit",
						false, valB);
	GU_ASSERT_RET_OK(status);

	memset(valA, 0, sizeof(valA));
	memset(valB, 0, sizeof(valB));

	status = gpiod_line_get_value_bulk(&bulkA, valA);
	GU_ASSERT_RET_OK(status);
	GU_ASSERT_EQ(valA[0], 1);
	GU_ASSERT_EQ(valA[1], 0);
	GU_ASSERT_EQ(valA[2], 0);
	GU_ASSERT_EQ(valA[3], 1);

	status = gpiod_line_get_value_bulk(&bulkB, valB);
	GU_ASSERT_RET_OK(status);
	GU_ASSERT_EQ(valB[0], 0);
	GU_ASSERT_EQ(valB[1], 1);
	GU_ASSERT_EQ(valB[2], 0);
	GU_ASSERT_EQ(valB[3], 1);

	gpiod_line_release_bulk(&bulkA);
	gpiod_line_release_bulk(&bulkB);
}
GU_DEFINE_TEST(line_request_bulk_output,
	       "gpiod_line_request_bulk_output() - good",
	       GU_LINES_UNNAMED, { 8, 8 });

static void line_request_bulk_different_chips(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chipA = NULL;
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chipB = NULL;
	struct gpiod_line_request_config req;
	struct gpiod_line *lineA0;
	struct gpiod_line *lineA1;
	struct gpiod_line *lineB0;
	struct gpiod_line *lineB1;
	struct gpiod_line_bulk bulk;
	int status;

	chipA = gpiod_chip_open(gu_chip_path(0));
	chipB = gpiod_chip_open(gu_chip_path(1));
	GU_ASSERT_NOT_NULL(chipA);
	GU_ASSERT_NOT_NULL(chipB);

	lineA0 = gpiod_chip_get_line(chipA, 0);
	lineA1 = gpiod_chip_get_line(chipA, 1);
	lineB0 = gpiod_chip_get_line(chipB, 0);
	lineB1 = gpiod_chip_get_line(chipB, 1);

	GU_ASSERT_NOT_NULL(lineA0);
	GU_ASSERT_NOT_NULL(lineA1);
	GU_ASSERT_NOT_NULL(lineB0);
	GU_ASSERT_NOT_NULL(lineB1);

	gpiod_line_bulk_init(&bulk);
	gpiod_line_bulk_add(&bulk, lineA0);
	gpiod_line_bulk_add(&bulk, lineA1);
	gpiod_line_bulk_add(&bulk, lineB0);
	gpiod_line_bulk_add(&bulk, lineB1);

	req.consumer = "gpiod-unit";
	req.direction = GPIOD_DIRECTION_INPUT;
	req.active_state = GPIOD_ACTIVE_STATE_HIGH;

	status = gpiod_line_request_bulk(&bulk, &req, NULL);
	GU_ASSERT_NOTEQ(status, 0);
	GU_ASSERT_EQ(gpiod_errno(), GPIOD_EBULKINCOH);
}
GU_DEFINE_TEST(line_request_bulk_different_chips,
	       "gpiod_line_request_bulk() - different chips",
	       GU_LINES_UNNAMED, { 8, 8 });

static void line_set_value(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;
	struct gpiod_line *line;
	int status;

	chip = gpiod_chip_open(gu_chip_path(0));
	GU_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 2);
	GU_ASSERT_NOT_NULL(line);

	status = gpiod_line_request_output(line, "gpiod-unit", false, 0);
	GU_ASSERT_RET_OK(status);

	GU_ASSERT_RET_OK(gpiod_line_set_value(line, 1));
	GU_ASSERT_EQ(gpiod_line_get_value(line), 1);
	GU_ASSERT_RET_OK(gpiod_line_set_value(line, 0));
	GU_ASSERT_EQ(gpiod_line_get_value(line), 0);

	gpiod_line_release(line);
}
GU_DEFINE_TEST(line_set_value,
	       "gpiod_line_set_value() - good",
	       GU_LINES_UNNAMED, { 8 });
