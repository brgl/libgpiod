/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/* Test cases for the gpioinfo program. */

#include "gpiod-test.h"

#include <stdio.h>

static void gpioinfo_dump_all_chips(void)
{
	test_tool_run("gpioinfo", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(),
				 test_build_str("%s - 4 lines:",
						test_chip_name(0)));
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(),
				 test_build_str("%s - 8 lines:",
						test_chip_name(1)));
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"\\s+line\\s+0:\\s+unnamed\\s+unused\\s+output\\s+active-high");
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"\\s+line\\s+7:\\s+unnamed\\s+unused\\s+output\\s+active-high");
}
TEST_DEFINE(gpioinfo_dump_all_chips,
	    "tools: gpioinfo - dump all chips",
	    0, { 4, 8 });

static void gpioinfo_dump_all_chips_one_exported(void)
{
	TEST_CLEANUP_CHIP struct gpiod_chip *chip = NULL;
	struct gpiod_line *line;
	int rv;

	chip = gpiod_chip_open(test_chip_path(1));
	TEST_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	TEST_ASSERT_NOT_NULL(line);

	rv = gpiod_line_request_input_flags(line, TEST_CONSUMER,
					    GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW);
	TEST_ASSERT_RET_OK(rv);

	test_tool_run("gpioinfo", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(),
				 test_build_str("%s - 4 lines:",
						test_chip_name(0)));
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(),
				 test_build_str("%s - 8 lines:",
						test_chip_name(1)));
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"\\s+line\\s+0:\\s+unnamed\\s+unused\\s+output\\s+active-high");
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"\\s+line\\s+7:\\s+unnamed\\s+\\\"" TEST_CONSUMER "\\\"\\s+input\\s+active-low");
}
TEST_DEFINE(gpioinfo_dump_all_chips_one_exported,
	    "tools: gpioinfo - dump all chips (one line exported)",
	    0, { 4, 8 });

static void gpioinfo_dump_one_chip(void)
{
	test_tool_run("gpioinfo", test_chip_name(1), (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_NOTCONT(test_tool_stdout(),
				test_build_str("%s - 8 lines:",
					       test_chip_name(0)));
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(),
				 test_build_str("%s - 4 lines:",
						test_chip_name(1)));
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"\\s+line\\s+0:\\s+unnamed\\s+unused\\s+output\\s+active-high");
	TEST_ASSERT_REGEX_NOMATCH(test_tool_stdout(),
				  "\\s+line\\s+7:\\s+unnamed\\s+unused\\s+output\\s+active-high");
}
TEST_DEFINE(gpioinfo_dump_one_chip,
	    "tools: gpioinfo - dump one chip",
	    0, { 8, 4 });

static void gpioinfo_dump_all_but_one_chip(void)
{
	test_tool_run("gpioinfo", test_chip_name(0),
		      test_chip_name(1), test_chip_name(3), (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_NOTCONT(test_tool_stdout(),
				test_build_str("%s - 8 lines:",
					       test_chip_name(2)));
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(),
				 test_build_str("%s - 4 lines:",
						test_chip_name(0)));
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(),
				 test_build_str("%s - 4 lines:",
						test_chip_name(1)));
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(),
				 test_build_str("%s - 4 lines:",
						test_chip_name(3)));
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"\\s+line\\s+0:\\s+unnamed\\s+unused\\s+output\\s+active-high");
	TEST_ASSERT_REGEX_NOMATCH(test_tool_stdout(),
				  "\\s+line\\s+7:\\s+unnamed\\s+unused\\s+output\\s+active-high");
}
TEST_DEFINE(gpioinfo_dump_all_but_one_chip,
	    "tools: gpioinfo - dump all but one chip",
	    0, { 4, 4, 8, 4 });

static void gpioinfo_inexistent_chip(void)
{
	test_tool_run("gpioinfo", "inexistent", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "looking up chip inexistent");
}
TEST_DEFINE(gpioinfo_inexistent_chip,
	    "tools: gpioinfo - inexistent chip",
	    0, { 8, 4 });
