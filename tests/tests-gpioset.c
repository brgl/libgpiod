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

#include <signal.h>
#include <unistd.h>

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

static void gpioset_set_lines_and_exit_active_low(void)
{
	unsigned int offsets[8];
	int rv, values[8];

	test_tool_run("gpioset", "--active-low", "gpiochip2",
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

	TEST_ASSERT_EQ(values[0], 1);
	TEST_ASSERT_EQ(values[1], 1);
	TEST_ASSERT_EQ(values[2], 0);
	TEST_ASSERT_EQ(values[3], 0);
	TEST_ASSERT_EQ(values[4], 0);
	TEST_ASSERT_EQ(values[5], 0);
	TEST_ASSERT_EQ(values[6], 1);
	TEST_ASSERT_EQ(values[7], 0);
}
TEST_DEFINE(gpioset_set_lines_and_exit_active_low,
	    "tools: gpioset - set lines and exit (active-low)",
	    0, { 8, 8, 8 });

static void gpioset_set_lines_and_exit_explicit_mode(void)
{
	unsigned int offsets[8];
	int rv, values[8];

	test_tool_run("gpioset", "--mode=exit", "gpiochip2",
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
TEST_DEFINE(gpioset_set_lines_and_exit_explicit_mode,
	    "tools: gpioset - set lines and exit (explicit mode argument)",
	    0, { 8, 8, 8 });

static void gpioset_set_some_lines_and_wait_for_enter(void)
{
	unsigned int offsets[5];
	int rv, values[5];

	test_tool_run("gpioset", "--mode=wait", "gpiochip2",
		      "1=0", "2=1", "5=1", "6=0", "7=1", (char *)NULL);
	test_tool_stdin_write("\n");
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());

	offsets[0] = 1;
	offsets[1] = 2;
	offsets[2] = 5;
	offsets[3] = 6;
	offsets[4] = 7;

	rv = gpiod_simple_get_value_multiple(TEST_CONSUMER, test_chip_name(2),
					     offsets, values, 5, false);
	TEST_ASSERT_RET_OK(rv);

	TEST_ASSERT_EQ(values[0], 0);
	TEST_ASSERT_EQ(values[1], 1);
	TEST_ASSERT_EQ(values[2], 1);
	TEST_ASSERT_EQ(values[3], 0);
	TEST_ASSERT_EQ(values[4], 1);
}
TEST_DEFINE(gpioset_set_some_lines_and_wait_for_enter,
	    "tools: gpioset - set some lines and wait for enter",
	    0, { 8, 8, 8 });

static void gpioset_set_some_lines_and_wait_for_signal(void)
{
	static const int signals[] = { SIGTERM, SIGINT };

	unsigned int offsets[5], i;
	int rv, values[5];

	for (i = 0; i < TEST_ARRAY_SIZE(signals); i++) {
		test_tool_run("gpioset", "--mode=signal", "gpiochip2",
			      "1=0", "2=1", "5=0", "6=0", "7=1", (char *)NULL);
		usleep(200000);
		test_tool_signal(signals[i]);
		test_tool_wait();

		TEST_ASSERT(test_tool_exited());
		TEST_ASSERT_RET_OK(test_tool_exit_status());
		TEST_ASSERT_NULL(test_tool_stdout());
		TEST_ASSERT_NULL(test_tool_stderr());

		offsets[0] = 1;
		offsets[1] = 2;
		offsets[2] = 5;
		offsets[3] = 6;
		offsets[4] = 7;

		rv = gpiod_simple_get_value_multiple(TEST_CONSUMER,
						     test_chip_name(2),
						     offsets, values,
						     5, false);
		TEST_ASSERT_RET_OK(rv);

		TEST_ASSERT_EQ(values[0], 0);
		TEST_ASSERT_EQ(values[1], 1);
		TEST_ASSERT_EQ(values[2], 0);
		TEST_ASSERT_EQ(values[3], 0);
		TEST_ASSERT_EQ(values[4], 1);
	}
}
TEST_DEFINE(gpioset_set_some_lines_and_wait_for_signal,
	    "tools: gpioset - set some lines and wait for signal",
	    0, { 8, 8, 8 });

static void gpioset_set_some_lines_and_wait_time(void)
{
	unsigned int offsets[3];
	int rv, values[3];

	test_tool_run("gpioset", "--mode=time", "--usec=100000", "gpiochip0",
		      "1=1", "2=0", "5=1", (char *)NULL);
	usleep(200000);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());

	offsets[0] = 1;
	offsets[1] = 2;
	offsets[2] = 5;

	rv = gpiod_simple_get_value_multiple(TEST_CONSUMER, test_chip_name(0),
					     offsets, values, 3, false);
	TEST_ASSERT_RET_OK(rv);

	TEST_ASSERT_EQ(values[0], 1);
	TEST_ASSERT_EQ(values[1], 0);
	TEST_ASSERT_EQ(values[2], 1);
}
TEST_DEFINE(gpioset_set_some_lines_and_wait_time,
	    "tools: gpioset - set some lines and wait for specified time",
	    0, { 8, 8, 8 });
