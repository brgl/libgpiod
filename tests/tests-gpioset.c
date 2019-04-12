// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

/* Test cases for the gpioset program. */

#include <signal.h>
#include <unistd.h>

#include "gpiod-test.h"

static void gpioset_set_lines_and_exit(void)
{
	test_tool_run("gpioset",
		      "--mode=signal", test_chip_name(2),
		      "0=0", "1=0", "2=1", "3=1",
		      "4=1", "5=1", "6=0", "7=1", (char *)NULL);
	usleep(200000);

	TEST_ASSERT_EQ(test_debugfs_get_value(2, 0), 0);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 1), 0);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 2), 1);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 3), 1);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 4), 1);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 5), 1);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 6), 0);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 7), 1);

	test_tool_signal(SIGTERM);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
}
TEST_DEFINE(gpioset_set_lines_and_exit,
	    "tools: gpioset - set lines and exit",
	    0, { 8, 8, 8 });

static void gpioset_set_lines_and_exit_active_low(void)
{
	test_tool_run("gpioset",
		      "--mode=signal", "--active-low", test_chip_name(2),
		      "0=0", "1=0", "2=1", "3=1",
		      "4=1", "5=1", "6=0", "7=1", (char *)NULL);
	usleep(200000);

	TEST_ASSERT_EQ(test_debugfs_get_value(2, 0), 1);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 1), 1);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 2), 0);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 3), 0);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 4), 0);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 5), 0);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 6), 1);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 7), 0);

	test_tool_signal(SIGTERM);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
}
TEST_DEFINE(gpioset_set_lines_and_exit_active_low,
	    "tools: gpioset - set lines and exit (active-low)",
	    0, { 8, 8, 8 });

static void gpioset_set_some_lines_and_wait_for_enter(void)
{
	test_tool_run("gpioset", "--mode=wait", test_chip_name(2),
		      "1=0", "2=1", "5=1", "6=0", "7=1", (char *)NULL);
	usleep(200000);

	TEST_ASSERT_EQ(test_debugfs_get_value(2, 1), 0);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 2), 1);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 5), 1);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 6), 0);
	TEST_ASSERT_EQ(test_debugfs_get_value(2, 7), 1);

	test_tool_stdin_write("\n");
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
}
TEST_DEFINE(gpioset_set_some_lines_and_wait_for_enter,
	    "tools: gpioset - set some lines and wait for enter",
	    0, { 8, 8, 8 });

static void gpioset_set_some_lines_and_wait_for_signal(void)
{
	static const int signals[] = { SIGTERM, SIGINT };

	unsigned int i;

	for (i = 0; i < TEST_ARRAY_SIZE(signals); i++) {
		test_tool_run("gpioset", "--mode=signal", test_chip_name(2),
			      "1=0", "2=1", "5=0", "6=0", "7=1", (char *)NULL);
		usleep(200000);

		TEST_ASSERT_EQ(test_debugfs_get_value(2, 1), 0);
		TEST_ASSERT_EQ(test_debugfs_get_value(2, 2), 1);
		TEST_ASSERT_EQ(test_debugfs_get_value(2, 5), 0);
		TEST_ASSERT_EQ(test_debugfs_get_value(2, 6), 0);
		TEST_ASSERT_EQ(test_debugfs_get_value(2, 7), 1);

		test_tool_signal(signals[i]);
		test_tool_wait();

		TEST_ASSERT(test_tool_exited());
		TEST_ASSERT_RET_OK(test_tool_exit_status());
		TEST_ASSERT_NULL(test_tool_stdout());
		TEST_ASSERT_NULL(test_tool_stderr());
	}
}
TEST_DEFINE(gpioset_set_some_lines_and_wait_for_signal,
	    "tools: gpioset - set some lines and wait for signal",
	    0, { 8, 8, 8 });

static void gpioset_set_some_lines_and_wait_time(void)
{
	test_tool_run("gpioset", "--mode=time",
		      "--usec=600000", "--sec=0", test_chip_name(0),
		      "1=1", "2=0", "5=1", (char *)NULL);
	usleep(200000);

	TEST_ASSERT_EQ(test_debugfs_get_value(0, 1), 1);
	TEST_ASSERT_EQ(test_debugfs_get_value(0, 2), 0);
	TEST_ASSERT_EQ(test_debugfs_get_value(0, 5), 1);

	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
}
TEST_DEFINE(gpioset_set_some_lines_and_wait_time,
	    "tools: gpioset - set some lines and wait for specified time",
	    0, { 8, 8, 8 });

static void gpioset_no_arguments(void)
{
	test_tool_run("gpioset", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "gpiochip must be specified");
}
TEST_DEFINE(gpioset_no_arguments,
	    "tools: gpioset - no arguments",
	    0, { });

static void gpioset_no_lines_specified(void)
{
	test_tool_run("gpioset", test_chip_name(1), (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "at least one GPIO line offset to value mapping must be specified");
}
TEST_DEFINE(gpioset_no_lines_specified,
	    "tools: gpioset - no lines specified",
	    0, { 4, 4 });

static void gpioset_too_many_lines_specified(void)
{
	test_tool_run("gpioset", test_chip_name(0),
		      "0=1", "1=1", "2=1", "3=1", "4=1", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "error setting the GPIO line values");
}
TEST_DEFINE(gpioset_too_many_lines_specified,
	    "tools: gpioset - too many lines specified",
	    0, { 4 });

static void gpioset_sec_usec_without_time(void)
{
	test_tool_run("gpioset", "--mode=exit", "--sec=1",
		      test_chip_name(0), "0=1", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "can't specify wait time in this mode");

	test_tool_run("gpioset", "--mode=exit", "--usec=100",
		      test_chip_name(0), "0=1", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "can't specify wait time in this mode");
}
TEST_DEFINE(gpioset_sec_usec_without_time,
	    "tools: gpioset - using --sec/--usec with mode other than 'time'",
	    0, { 4 });

static void gpioset_invalid_mapping(void)
{
	test_tool_run("gpioset", test_chip_name(0), "0=c", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "invalid offset<->value mapping");
}
TEST_DEFINE(gpioset_invalid_mapping,
	    "tools: gpioset - invalid offset<->value mapping",
	    0, { 4 });

static void gpioset_invalid_value(void)
{
	test_tool_run("gpioset", test_chip_name(0), "0=3", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(), "value must be 0 or 1");
}
TEST_DEFINE(gpioset_invalid_value,
	    "tools: gpioset - value different than 0 or 1",
	    0, { 4 });

static void gpioset_invalid_offset(void)
{
	test_tool_run("gpioset", test_chip_name(0),
		      "4000000000=1", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(), "invalid offset");
}
TEST_DEFINE(gpioset_invalid_offset,
	    "tools: gpioset - invalid offset",
	    0, { 4 });

static void gpioset_daemonize_in_wrong_mode(void)
{
	test_tool_run("gpioset", "--background",
		      test_chip_name(0), "0=1", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "can't daemonize in this mode");
}
TEST_DEFINE(gpioset_daemonize_in_wrong_mode,
	    "tools: gpioset - daemonize in wrong mode",
	    0, { 4 });
