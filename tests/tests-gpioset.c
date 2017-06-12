/*
 * Test cases for the gpioset tool.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "gpiod-test.h"

static void gpioset_set_lines_and_exit(void)
{
	unsigned int offsets[8];
	int rv, values[8];

	test_tool_run("gpioset", "gpiochip2",
		      "0=0", "1=0", "2=1", "3=1",
		      "4=1", "5=1", "6=0", "7=1", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());

	offsets[0] = 0;
	offsets[1] = 1;
	offsets[2] = 2;
	offsets[3] = 3;
	offsets[4] = 4;
	offsets[5] = 5;
	offsets[6] = 6;
	offsets[7] = 7;

	rv = gpiod_simple_get_value_multiple(TEST_CONSUMER, test_chip_name(2),
					     offsets, values, 8, false);
	TEST_ASSERT_RET_OK(rv);

	TEST_ASSERT_EQ(values[0], 0);
	TEST_ASSERT_EQ(values[1], 0);
	TEST_ASSERT_EQ(values[2], 1);
	TEST_ASSERT_EQ(values[3], 1);
	TEST_ASSERT_EQ(values[4], 1);
	TEST_ASSERT_EQ(values[5], 1);
	TEST_ASSERT_EQ(values[6], 0);
	TEST_ASSERT_EQ(values[7], 1);
}
TEST_DEFINE(gpioset_set_lines_and_exit,
	    "tools: gpioset - set lines and exit",
	    0, { 8, 8, 8 });
