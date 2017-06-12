/*
 * Test cases for the gpioget tool.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "gpiod-test.h"

static void gpioget_read_all_lines(void)
{
	unsigned int offsets[4];
	int rv, values[4];

	test_tool_run("gpioget", "gpiochip1",
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

	rv = gpiod_simple_set_value_multiple(TEST_CONSUMER, test_chip_name(1),
					     offsets, values, 4, false,
					     NULL, NULL);
	TEST_ASSERT_RET_OK(rv);

	test_tool_run("gpioget", "gpiochip1",
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
	test_tool_run("gpioget", "gpiochip1", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "at least one gpio line offset must be specified");
}
TEST_DEFINE(gpioget_no_lines_specified,
	    "tools: gpioget - no lines specified",
	    0, { 4, 4 });
