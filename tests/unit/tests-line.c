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
	GU_CLEANUP(gu_release_line) struct gpiod_line *line_0 = NULL;
	GU_CLEANUP(gu_release_line) struct gpiod_line *line_1 = NULL;
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
}
GU_DEFINE_TEST(line_request_output,
	       "line_request_output()",
	       GU_LINES_UNNAMED, { 8 });

static void line_set_value(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;
	GU_CLEANUP(gu_release_line) struct gpiod_line *line = NULL;
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
}
GU_DEFINE_TEST(line_set_value,
	       "line_set_value()",
	       GU_LINES_UNNAMED, { 8 });
