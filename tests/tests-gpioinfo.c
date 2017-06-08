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

static void gpioinfo_good(void)
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
TEST_DEFINE(gpioinfo_good,
	    "tools: gpioinfo - good",
	    0, { 4, 8 });
