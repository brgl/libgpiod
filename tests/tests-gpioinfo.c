/*
 * Test cases for the gpioinfo tool.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "gpiod-test.h"

#include <stdio.h>

static void gpioinfo_dump_all_chips(void)
{
	test_tool_run("gpioinfo", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), "gpiochip0 - 4 lines:");
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), "gpiochip1 - 8 lines:");
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
	TEST_CLEANUP(test_close_chip) struct gpiod_chip *chip = NULL;
	TEST_CLEANUP(test_free_str) char *ptrn = NULL;
	struct gpiod_line *line;
	int rv;

	rv = asprintf(&ptrn,
		      "\\s+line\\s+7:\\s+unnamed\\s+\\\"%s\\\"\\s+input\\s+active-low",
		      TEST_CONSUMER);
	TEST_ASSERT(rv > 0);

	chip = gpiod_chip_open(test_chip_path(1));
	TEST_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	TEST_ASSERT_NOT_NULL(line);

	rv = gpiod_line_request_input(line, TEST_CONSUMER, true);
	TEST_ASSERT_RET_OK(rv);

	test_tool_run("gpioinfo", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), "gpiochip0 - 4 lines:");
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), "gpiochip1 - 8 lines:");
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"\\s+line\\s+0:\\s+unnamed\\s+unused\\s+output\\s+active-high");
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(), ptrn);
}
TEST_DEFINE(gpioinfo_dump_all_chips_one_exported,
	    "tools: gpioinfo - dump all chips (one line exported)",
	    0, { 4, 8 });

static void gpioinfo_dump_one_chip(void)
{
	test_tool_run("gpioinfo", "gpiochip1", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_NOTCONT(test_tool_stdout(), "gpiochip0 - 8 lines:");
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), "gpiochip1 - 4 lines:");
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
	test_tool_run("gpioinfo", "gpiochip0",
		      "gpiochip1", "gpiochip3", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_NOTCONT(test_tool_stdout(), "gpiochip2 - 8 lines:");
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), "gpiochip0 - 4 lines:");
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), "gpiochip1 - 4 lines:");
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), "gpiochip3 - 4 lines:");
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
