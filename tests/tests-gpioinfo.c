/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

/* Test cases for the gpioinfo program. */

#include "gpiod-test.h"

#include <stdio.h>

static void gpioinfo_dump_all_chips(void)
{
	TEST_CLEANUP(test_free_str) char *ptrn_line0 = NULL;
	TEST_CLEANUP(test_free_str) char *ptrn_line1 = NULL;
	int rv0, rv1;

	rv0 = asprintf(&ptrn_line0, "%s - 4 lines:", test_chip_name(0));
	rv1 = asprintf(&ptrn_line1, "%s - 8 lines:", test_chip_name(1));
	TEST_ASSERT(rv0 > 0);
	TEST_ASSERT(rv1 > 0);

	test_tool_run("gpioinfo", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), ptrn_line0);
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), ptrn_line1);
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
	TEST_CLEANUP(test_free_str) char *ptrn_line0 = NULL;
	TEST_CLEANUP(test_free_str) char *ptrn_line1 = NULL;
	TEST_CLEANUP(test_free_str) char *ptrn_lineinfo = NULL;
	struct gpiod_line *line;
	int rv0, rv1, rv2;

	rv0 = asprintf(&ptrn_line0, "%s - 4 lines:", test_chip_name(0));
	rv1 = asprintf(&ptrn_line1, "%s - 8 lines:", test_chip_name(1));
	rv2 = asprintf(&ptrn_lineinfo,
		      "\\s+line\\s+7:\\s+unnamed\\s+\\\"%s\\\"\\s+input\\s+active-low",
		      TEST_CONSUMER);
	TEST_ASSERT(rv0 > 0);
	TEST_ASSERT(rv1 > 0);
	TEST_ASSERT(rv2 > 0);

	chip = gpiod_chip_open(test_chip_path(1));
	TEST_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	TEST_ASSERT_NOT_NULL(line);

	rv2 = gpiod_line_request_input_flags(line, TEST_CONSUMER,
					    GPIOD_LINE_REQUEST_ACTIVE_LOW);
	TEST_ASSERT_RET_OK(rv2);

	test_tool_run("gpioinfo", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), ptrn_line0);
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), ptrn_line1);
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"\\s+line\\s+0:\\s+unnamed\\s+unused\\s+output\\s+active-high");
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(), ptrn_lineinfo);
}
TEST_DEFINE(gpioinfo_dump_all_chips_one_exported,
	    "tools: gpioinfo - dump all chips (one line exported)",
	    0, { 4, 8 });

static void gpioinfo_dump_one_chip(void)
{
	TEST_CLEANUP(test_free_str) char *ptrn_line0 = NULL;
	TEST_CLEANUP(test_free_str) char *ptrn_line1 = NULL;
	int rv0, rv1;

	rv0 = asprintf(&ptrn_line0, "%s - 8 lines:", test_chip_name(0));
	rv1 = asprintf(&ptrn_line1, "%s - 4 lines:", test_chip_name(1));
	TEST_ASSERT(rv0 > 0);
	TEST_ASSERT(rv1 > 0);

	test_tool_run("gpioinfo", test_chip_name(1), (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_NOTCONT(test_tool_stdout(), ptrn_line0);
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), ptrn_line1);
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
	TEST_CLEANUP(test_free_str) char *ptrn_line0 = NULL;
	TEST_CLEANUP(test_free_str) char *ptrn_line1 = NULL;
	TEST_CLEANUP(test_free_str) char *ptrn_line2 = NULL;
	TEST_CLEANUP(test_free_str) char *ptrn_line3 = NULL;
	int rv0, rv1, rv2, rv3;

	rv0 = asprintf(&ptrn_line0, "%s - 4 lines:", test_chip_name(0));
	rv1 = asprintf(&ptrn_line1, "%s - 4 lines:", test_chip_name(1));
	rv2 = asprintf(&ptrn_line2, "%s - 8 lines:", test_chip_name(2));
	rv3 = asprintf(&ptrn_line3, "%s - 4 lines:", test_chip_name(3));
	TEST_ASSERT(rv0 > 0);
	TEST_ASSERT(rv1 > 0);
	TEST_ASSERT(rv2 > 0);
	TEST_ASSERT(rv3 > 0);

	test_tool_run("gpioinfo", test_chip_name(0),
		      test_chip_name(1), test_chip_name(3), (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_NOTCONT(test_tool_stdout(), ptrn_line2);
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), ptrn_line0);
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), ptrn_line1);
	TEST_ASSERT_STR_CONTAINS(test_tool_stdout(), ptrn_line3);
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
