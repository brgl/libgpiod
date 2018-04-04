// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/* Test cases for the gpiomon program. */

#include "gpiod-test.h"

#include <signal.h>
#include <unistd.h>

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

static void gpiomon_single_rising_edge_event_active_low(void)
{
	test_tool_run("gpiomon", "--rising-edge", "--num-events=1",
		      "--active-low", test_chip_name(1), "4", (char *)NULL);
	test_set_event(1, 4, TEST_EVENT_RISING, 200);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"event\\:\\s+RISING\\s+EDGE\\s+offset\\:\\s+4\\s+timestamp:\\s+\\[[0-9]+\\.[0-9]+\\]");
}
TEST_DEFINE(gpiomon_single_rising_edge_event_active_low,
	    "tools: gpiomon - single rising edge event (active-low)",
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

static void gpiomon_falling_edge_events_sigint(void)
{
	test_tool_run("gpiomon", "--falling-edge",
		      test_chip_name(0), "4", (char *)NULL);
	test_set_event(0, 4, TEST_EVENT_FALLING, 100);
	usleep(200000);
	test_tool_signal(SIGINT);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(),
				"event\\:\\s+FALLING\\s+EDGE\\s+offset\\:\\s+4\\s+timestamp:\\s+\\[[0-9]+\\.[0-9]+\\]");
}
TEST_DEFINE(gpiomon_falling_edge_events_sigint,
	    "tools: gpiomon - receive falling edge events and kill with SIGINT",
	    0, { 8, 8 });

static void gpiomon_both_events_sigterm(void)
{
	test_tool_run("gpiomon", "--falling-edge", "--rising-edge",
		      test_chip_name(0), "4", (char *)NULL);
	test_set_event(0, 4, TEST_EVENT_ALTERNATING, 100);
	usleep(300000);
	test_tool_signal(SIGTERM);
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
TEST_DEFINE(gpiomon_both_events_sigterm,
	    "tools: gpiomon - receive both types of events and kill with SIGTERM",
	    0, { 8, 8 });

static void gpiomon_ignore_falling_edge(void)
{
	test_tool_run("gpiomon", "--rising-edge",
		      test_chip_name(0), "4", (char *)NULL);
	test_set_event(0, 4, TEST_EVENT_FALLING, 100);
	usleep(300000);
	test_tool_signal(SIGTERM);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
}
TEST_DEFINE(gpiomon_ignore_falling_edge,
	    "tools: gpiomon - wait for rising edge events, ignore falling edge",
	    0, { 8, 8 });

static void gpiomon_watch_multiple_lines(void)
{
	test_tool_run("gpiomon", "--format=%o", test_chip_name(0),
		      "1", "2", "3", "4", "5", (char *)NULL);
	test_set_event(0, 2, TEST_EVENT_ALTERNATING, 100);
	usleep(150000);
	test_set_event(0, 3, TEST_EVENT_ALTERNATING, 100);
	usleep(150000);
	test_set_event(0, 4, TEST_EVENT_ALTERNATING, 100);
	usleep(150000);
	test_tool_signal(SIGTERM);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "2\n3\n4\n");

}
TEST_DEFINE(gpiomon_watch_multiple_lines,
	    "tools: gpiomon - watch multiple lines",
	    0, { 8, 8 });

static void gpiomon_watch_multiple_lines_not_in_order(void)
{
	test_tool_run("gpiomon", "--format=%o", test_chip_name(0),
		      "5", "2", "7", "1", "6", (char *)NULL);
	test_set_event(0, 2, TEST_EVENT_ALTERNATING, 100);
	usleep(150000);
	test_set_event(0, 1, TEST_EVENT_ALTERNATING, 100);
	usleep(150000);
	test_set_event(0, 6, TEST_EVENT_ALTERNATING, 100);
	usleep(150000);
	test_tool_signal(SIGTERM);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "2\n1\n6\n");

}
TEST_DEFINE(gpiomon_watch_multiple_lines_not_in_order,
	    "tools: gpiomon - watch multiple lines (offsets not in order)",
	    0, { 8, 8 });

static void gpiomon_request_the_same_line_twice(void)
{
	test_tool_run("gpiomon", test_chip_name(0), "2", "2", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "error waiting for events");
}
TEST_DEFINE(gpiomon_request_the_same_line_twice,
	    "tools: gpiomon - request the same line twice",
	    0, { 8, 8 });

static void gpiomon_no_arguments(void)
{
	test_tool_run("gpiomon", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "gpiochip must be specified");
}
TEST_DEFINE(gpiomon_no_arguments,
	    "tools: gpiomon - no arguments",
	    0, { });

static void gpiomon_line_not_specified(void)
{
	test_tool_run("gpiomon", test_chip_name(1), (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "GPIO line offset must be specified");
}
TEST_DEFINE(gpiomon_line_not_specified,
	    "tools: gpiomon - line not specified",
	    0, { 4, 4 });

static void gpiomon_line_out_of_range(void)
{
	test_tool_run("gpiomon", test_chip_name(0), "4", (char *)NULL);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_EQ(test_tool_exit_status(), 1);
	TEST_ASSERT_NULL(test_tool_stdout());
	TEST_ASSERT_NOT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_CONTAINS(test_tool_stderr(),
				 "error waiting for events");
}
TEST_DEFINE(gpiomon_line_out_of_range,
	    "tools: gpiomon - line out of range",
	    0, { 4 });

static void gpiomon_custom_format_event_and_offset(void)
{
	test_tool_run("gpiomon", "--num-events=1", "--format=%e %o",
		      test_chip_name(0), "3", (char *)NULL);
	test_set_event(0, 3, TEST_EVENT_RISING, 100);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "1 3\n");
}
TEST_DEFINE(gpiomon_custom_format_event_and_offset,
	    "tools: gpiomon - custom output format: event and offset",
	    0, { 8, 8 });

static void gpiomon_custom_format_event_and_offset_joined(void)
{
	test_tool_run("gpiomon", "--num-events=1", "--format=%e%o",
		      test_chip_name(0), "3", (char *)NULL);
	test_set_event(0, 3, TEST_EVENT_RISING, 100);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "13\n");
}
TEST_DEFINE(gpiomon_custom_format_event_and_offset_joined,
	    "tools: gpiomon - custom output format: event and offset, joined strings",
	    0, { 8, 8 });

static void gpiomon_custom_format_timestamp(void)
{
	test_tool_run("gpiomon", "--num-events=1", "--format=%e %o %s.%n",
		      test_chip_name(0), "3", (char *)NULL);
	test_set_event(0, 3, TEST_EVENT_RISING, 100);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_REGEX_MATCH(test_tool_stdout(), "1 3 [0-9]+\\.[0-9]+");
}
TEST_DEFINE(gpiomon_custom_format_timestamp,
	    "tools: gpiomon - custom output format: timestamp",
	    0, { 8, 8 });

static void gpiomon_custom_format_double_percent_sign(void)
{
	test_tool_run("gpiomon", "--num-events=1", "--format=%%",
		      test_chip_name(0), "3", (char *)NULL);
	test_set_event(0, 3, TEST_EVENT_RISING, 100);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "%\n");
}
TEST_DEFINE(gpiomon_custom_format_double_percent_sign,
	    "tools: gpiomon - custom output format: double percent sign",
	    0, { 8, 8 });

static void gpiomon_custom_format_double_percent_sign_and_spec(void)
{
	test_tool_run("gpiomon", "--num-events=1", "--format=%%e",
		      test_chip_name(0), "3", (char *)NULL);
	test_set_event(0, 3, TEST_EVENT_RISING, 100);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "%e\n");
}
TEST_DEFINE(gpiomon_custom_format_double_percent_sign_and_spec,
	    "tools: gpiomon - custom output format: double percent sign with specifier",
	    0, { 8, 8 });

static void gpiomon_custom_format_single_percent_sign(void)
{
	test_tool_run("gpiomon", "--num-events=1", "--format=%",
		      test_chip_name(0), "3", (char *)NULL);
	test_set_event(0, 3, TEST_EVENT_RISING, 100);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "%\n");
}
TEST_DEFINE(gpiomon_custom_format_single_percent_sign,
	    "tools: gpiomon - custom output format: single percent sign",
	    0, { 8, 8 });

static void gpiomon_custom_format_single_percent_sign_between_chars(void)
{
	test_tool_run("gpiomon", "--num-events=1", "--format=foo % bar",
		      test_chip_name(0), "3", (char *)NULL);
	test_set_event(0, 3, TEST_EVENT_RISING, 100);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "foo % bar\n");
}
TEST_DEFINE(gpiomon_custom_format_single_percent_sign_between_chars,
	    "tools: gpiomon - custom output format: single percent sign between other characters",
	    0, { 8, 8 });

static void gpiomon_custom_format_unknown_specifier(void)
{
	test_tool_run("gpiomon", "--num-events=1", "--format=%x",
		      test_chip_name(0), "3", (char *)NULL);
	test_set_event(0, 3, TEST_EVENT_RISING, 100);
	test_tool_wait();

	TEST_ASSERT(test_tool_exited());
	TEST_ASSERT_RET_OK(test_tool_exit_status());
	TEST_ASSERT_NOT_NULL(test_tool_stdout());
	TEST_ASSERT_NULL(test_tool_stderr());
	TEST_ASSERT_STR_EQ(test_tool_stdout(), "%x\n");
}
TEST_DEFINE(gpiomon_custom_format_unknown_specifier,
	    "tools: gpiomon - custom output format: unknown specifier",
	    0, { 8, 8 });
