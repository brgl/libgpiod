/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 */

/* Test cases for the gpioget program. */

#include "gpiod-test.h"

static void gpioget_read_all_lines(void)
{
	unsigned int offsets[4];
	int rv, values[4];

	test_tool_run("gpioget", test_chip_name(1),
		      "0", "1", "2", "3", "4", "5", "6", "7", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "0 0 0 0 0 0 0 0\n");

	offsets[0] = 2;
	offsets[1] = 3;
	offsets[2] = 5;
	offsets[3] = 7;

	values[0] = values[1] = values[2] = values[3] = 1;

	rv = gpiod_ctxless_set_value_multiple(test_chip_name(1), offsets,
					      values, 4, false, TEST_CONSUMER,
					      NULL, NULL);
	TEST_ASSERT_RET_OK(rv);

	test_tool_run("gpioget", test_chip_name(1),
		      "0", "1", "2", "3", "4", "5", "6", "7", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "0 0 1 1 0 1 0 1\n");
}
TEST_DEFINE(gpioget_read_all_lines,
	    "tools: gpioget - read all lines",
	    0, { 8, 8, 8 });

static void gpioget_read_all_lines_active_low(void)
{
	unsigned int offsets[4];
	int rv, values[4];

	test_tool_run("gpioget", "--active-low", test_chip_name(1),
		      "0", "1", "2", "3", "4", "5", "6", "7", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "1 1 1 1 1 1 1 1\n");

	offsets[0] = 2;
	offsets[1] = 3;
	offsets[2] = 5;
	offsets[3] = 7;

	values[0] = values[1] = values[2] = values[3] = 1;

	rv = gpiod_ctxless_set_value_multiple(test_chip_name(1), offsets,
					      values, 4, false, TEST_CONSUMER,
					      NULL, NULL);
	TEST_ASSERT_RET_OK(rv);

	test_tool_run("gpioget", "--active-low", test_chip_name(1),
		      "0", "1", "2", "3", "4", "5", "6", "7", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "1 1 0 0 1 0 1 0\n");
}
TEST_DEFINE(gpioget_read_all_lines_active_low,
	    "tools: gpioget - read all lines (active-low)",
	    0, { 8, 8, 8 });

static void gpioget_read_some_lines(void)
{
	unsigned int offsets[3];
	int rv, values[3];

	test_tool_run("gpioget", test_chip_name(1),
		      "0", "1", "4", "6", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "0 0 0 0\n");

	offsets[0] = 1;
	offsets[1] = 4;
	offsets[2] = 6;

	values[0] = values[1] = values[2] = 1;

	rv = gpiod_ctxless_set_value_multiple(test_chip_name(1), offsets,
					      values, 3, false, TEST_CONSUMER,
					      NULL, NULL);
	TEST_ASSERT_RET_OK(rv);

	test_tool_run("gpioget", test_chip_name(1),
			      "0", "1", "4", "6", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "0 1 1 1\n");
}
TEST_DEFINE(gpioget_read_some_lines,
	    "tools: gpioget - read some lines",
	    0, { 8, 8, 8 });

static void gpioget_no_arguments(void)
{
	test_tool_run("gpioget", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "gpiochip must be specified");
}
TEST_DEFINE(gpioget_no_arguments,
	    "tools: gpioget - no arguments",
	    0, { });

static void gpioget_no_lines_specified(void)
{
	test_tool_run("gpioget", test_chip_name(1), (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "at least one GPIO line offset must be specified");
}
TEST_DEFINE(gpioget_no_lines_specified,
	    "tools: gpioget - no lines specified",
	    0, { 4, 4 });

static void gpioget_too_many_lines_specified(void)
{
	test_tool_run("gpioget", test_chip_name(0),
		      "0", "1", "2", "3", "4", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "error reading GPIO values");
}
TEST_DEFINE(gpioget_too_many_lines_specified,
	    "tools: gpioget - too many lines specified",
	    0, { 4 });
