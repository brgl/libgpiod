/*
 * Test cases for the gpiomon tool.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "gpiod-test.h"

static void gpiomon_single_rising_edge_event(void)
{
	test_tool_run("gpiomon", "--rising-edge", "--num-events=1",
		      test_chip_name(1), "4", (char *)NULL);
	test_set_event(1, 4, TEST_EVENT_RISING, 200);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"event\\:\\s+RISING\\s+EDGE\\s+offset\\:\\s+4\\s+timestamp:\\s+\\[[0-9]+\\.[0-9]+\\]");
}
TEST_DEFINE(gpiomon_single_rising_edge_event,
	    "tools: gpiomon - single rising edge event",
	    0, { 8, 8 });

static void gpiomon_single_rising_edge_event_silent(void)
{
	test_tool_run("gpiomon", "--rising-edge", "--num-events=1",
		      "--silent", test_chip_name(1), "4", (char *)NULL);
	test_set_event(1, 4, TEST_EVENT_RISING, 200);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
}
TEST_DEFINE(gpiomon_single_rising_edge_event_silent,
	    "tools: gpiomon - single rising edge event (silent mode)",
	    0, { 8, 8 });

static void gpiomon_four_alternating_events(void)
{
	test_tool_run("gpiomon", "--num-events=4",
		      test_chip_name(1), "4", (char *)NULL);
	test_set_event(1, 4, TEST_EVENT_ALTERNATING, 100);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"event\\:\\s+FALLING\\s+EDGE\\s+offset\\:\\s+4\\s+timestamp:\\s+\\[[0-9]+\\.[0-9]+\\]");
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"event\\:\\s+RISING\\s+EDGE\\s+offset\\:\\s+4\\s+timestamp:\\s+\\[[0-9]+\\.[0-9]+\\]");
}
TEST_DEFINE(gpiomon_four_alternating_events,
	    "tools: gpiomon - four alternating events",
	    0, { 8, 8 });
