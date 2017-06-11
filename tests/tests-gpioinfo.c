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

static void gpioinfo_simple(void)
{
	test_gpiotool_run("gpioinfo", (char *)NULL);
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
TEST_DEFINE(gpioinfo_simple,
	    "tools: gpioinfo - simple",
	    0, { 4, 8 });

static void gpioinfo_one_exported(void)
{
	TEST_CLEANUP(test_close_chip) struct gpiod_chip *chip = NULL;
	TEST_CLEANUP(test_free_str) char *ptrn;
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

	test_gpiotool_run("gpioinfo", (char *)NULL);
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
TEST_DEFINE(gpioinfo_one_exported,
	    "tools: gpioinfo - exported line",
	    0, { 4, 8 });
