/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 */

/* Test cases for the high-level API. */

#include "gpiod-test.h"

#include <errno.h>

static void ctxless_set_get_value(void)
{
	int ret;

	ret = gpiod_ctxless_get_value(test_chip_name(0), 3,
				      false, TEST_CONSUMER);
	TEST_ASSERT_EQ(ret, 0);

	ret = gpiod_ctxless_set_value(test_chip_name(0), 3, 1,
				      false, TEST_CONSUMER, NULL, NULL);
	TEST_ASSERT_RET_OK(ret);

	ret = gpiod_ctxless_get_value(test_chip_name(0), 3,
				      false, TEST_CONSUMER);
	TEST_ASSERT_EQ(ret, 1);
}
TEST_DEFINE(ctxless_set_get_value,
	    "ctxless set/get value - single line",
	    0, { 8 });

static void ctxless_set_get_value_multiple(void)
{
	unsigned int offsets[] = { 0, 1, 2, 3, 4, 5, 6, 12, 13, 15 };
	int values[10], rv;

	rv = gpiod_ctxless_get_value_multiple(test_chip_name(0), offsets,
					      values, 10, false, TEST_CONSUMER);
	TEST_ASSERT_RET_OK(rv);

	TEST_ASSERT_EQ(values[0], 0);
	TEST_ASSERT_EQ(values[1], 0);
	TEST_ASSERT_EQ(values[2], 0);
	TEST_ASSERT_EQ(values[3], 0);
	TEST_ASSERT_EQ(values[4], 0);
	TEST_ASSERT_EQ(values[5], 0);
	TEST_ASSERT_EQ(values[6], 0);
	TEST_ASSERT_EQ(values[7], 0);
	TEST_ASSERT_EQ(values[8], 0);
	TEST_ASSERT_EQ(values[9], 0);

	values[0] = 1;
	values[1] = 1;
	values[2] = 1;
	values[3] = 0;
	values[4] = 0;
	values[5] = 1;
	values[6] = 0;
	values[7] = 1;
	values[8] = 0;
	values[9] = 0;

	rv = gpiod_ctxless_set_value_multiple(test_chip_name(0), offsets,
					      values, 10, false, TEST_CONSUMER,
					      NULL, NULL);
	TEST_ASSERT_RET_OK(rv);

	rv = gpiod_ctxless_get_value_multiple(test_chip_name(0), offsets,
					      values, 10, false, TEST_CONSUMER);
	TEST_ASSERT_RET_OK(rv);

	TEST_ASSERT_EQ(values[0], 1);
	TEST_ASSERT_EQ(values[1], 1);
	TEST_ASSERT_EQ(values[2], 1);
	TEST_ASSERT_EQ(values[3], 0);
	TEST_ASSERT_EQ(values[4], 0);
	TEST_ASSERT_EQ(values[5], 1);
	TEST_ASSERT_EQ(values[6], 0);
	TEST_ASSERT_EQ(values[7], 1);
	TEST_ASSERT_EQ(values[8], 0);
	TEST_ASSERT_EQ(values[9], 0);
}
TEST_DEFINE(ctxless_set_get_value_multiple,
	    "ctxless set/get value - multiple lines",
	    0, { 16 });

static void ctxless_get_value_multiple_max_lines(void)
{
	unsigned int offsets[GPIOD_LINE_BULK_MAX_LINES + 1];
	int values[GPIOD_LINE_BULK_MAX_LINES + 1], ret;

	ret = gpiod_ctxless_get_value_multiple(test_chip_name(0), offsets,
					       values,
					       GPIOD_LINE_BULK_MAX_LINES + 1,
					       false, TEST_CONSUMER);
	TEST_ASSERT_NOTEQ(ret, 0);
	TEST_ASSERT_ERRNO_IS(EINVAL);
}
TEST_DEFINE(ctxless_get_value_multiple_max_lines,
	    "gpiod_ctxless_get_value_multiple() exceed max lines",
	    0, { 128 });

static void ctxless_set_value_multiple_max_lines(void)
{
	unsigned int offsets[GPIOD_LINE_BULK_MAX_LINES + 1];
	int values[GPIOD_LINE_BULK_MAX_LINES + 1], ret;

	ret = gpiod_ctxless_set_value_multiple(test_chip_name(0), offsets,
					       values,
					       GPIOD_LINE_BULK_MAX_LINES + 1,
					       false, TEST_CONSUMER,
					       NULL, NULL);
	TEST_ASSERT_NOTEQ(ret, 0);
	TEST_ASSERT_ERRNO_IS(EINVAL);
}
TEST_DEFINE(ctxless_set_value_multiple_max_lines,
	    "gpiod_ctxless_set_value_multiple() exceed max lines",
	    0, { 128 });

struct ctxless_event_data {
	bool got_rising_edge;
	bool got_falling_edge;
	unsigned int offset;
	unsigned int count;
};

static int ctxless_event_cb(int evtype, unsigned int offset,
			   const struct timespec *ts TEST_UNUSED, void *data)
{
	struct ctxless_event_data *evdata = data;

	if (evtype == GPIOD_CTXLESS_EVENT_CB_RISING_EDGE)
		evdata->got_rising_edge = true;
	else if (evtype == GPIOD_CTXLESS_EVENT_CB_FALLING_EDGE)
		evdata->got_falling_edge = true;

	evdata->offset = offset;

	return ++evdata->count == 2 ? GPIOD_CTXLESS_EVENT_CB_RET_STOP
				    : GPIOD_CTXLESS_EVENT_CB_RET_OK;
}

static void ctxless_event_loop(void)
{
	struct ctxless_event_data evdata = { false, false, 0, 0 };
	struct timespec ts = { 1, 0 };
	int status;

	test_set_event(0, 3, TEST_EVENT_ALTERNATING, 100);

	status = gpiod_ctxless_event_loop(test_chip_name(0), 3, false,
					  TEST_CONSUMER, &ts, NULL,
					  ctxless_event_cb, &evdata);

	TEST_ASSERT_RET_OK(status);
	TEST_ASSERT(evdata.got_rising_edge);
	TEST_ASSERT(evdata.got_falling_edge);
	TEST_ASSERT_EQ(evdata.count, 2);
	TEST_ASSERT_EQ(evdata.offset, 3);
}
TEST_DEFINE(ctxless_event_loop,
	    "gpiod_ctxless_event_loop() - single event",
	    0, { 8 });

static void ctxless_event_loop_multiple(void)
{
	struct ctxless_event_data evdata = { false, false, 0, 0 };
	struct timespec ts = { 1, 0 };
	unsigned int offsets[4];
	int status;

	offsets[0] = 2;
	offsets[1] = 3;
	offsets[2] = 5;
	offsets[3] = 6;

	test_set_event(0, 3, TEST_EVENT_ALTERNATING, 100);

	status = gpiod_ctxless_event_loop_multiple(test_chip_name(0), offsets,
						   4, false, TEST_CONSUMER,
						   &ts, NULL, ctxless_event_cb,
						   &evdata);

	TEST_ASSERT_RET_OK(status);
	TEST_ASSERT(evdata.got_rising_edge);
	TEST_ASSERT(evdata.got_falling_edge);
	TEST_ASSERT_EQ(evdata.count, 2);
	TEST_ASSERT_EQ(evdata.offset, 3);
}
TEST_DEFINE(ctxless_event_loop_multiple,
	    "gpiod_ctxless_event_loop_multiple() - single event",
	    0, { 8 });

static int error_event_cb(int evtype TEST_UNUSED,
			  unsigned int offset TEST_UNUSED,
			  const struct timespec *ts TEST_UNUSED,
			  void *data TEST_UNUSED)
{
	errno = ENOTBLK;

	return GPIOD_CTXLESS_EVENT_CB_RET_ERR;
}

static void ctxless_event_loop_indicate_error(void)
{
	struct timespec ts = { 1, 0 };
	int rv;

	test_set_event(0, 3, TEST_EVENT_ALTERNATING, 100);

	rv = gpiod_ctxless_event_loop(test_chip_name(0), 3, false,
				      TEST_CONSUMER, &ts, NULL,
				      error_event_cb, NULL);

	TEST_ASSERT_EQ(rv, -1);
	TEST_ASSERT_ERRNO_IS(ENOTBLK);
}
TEST_DEFINE(ctxless_event_loop_indicate_error,
	    "gpiod_ctxless_event_loop() - error in callback",
	    0, { 8 });

static void ctxless_event_loop_indicate_error_timeout(void)
{
	struct timespec ts = { 0, 100000 };
	int rv;

	rv = gpiod_ctxless_event_loop(test_chip_name(0), 3, false,
				      TEST_CONSUMER, &ts, NULL,
				      error_event_cb, NULL);

	TEST_ASSERT_EQ(rv, -1);
	TEST_ASSERT_ERRNO_IS(ENOTBLK);
}
TEST_DEFINE(ctxless_event_loop_indicate_error_timeout,
	    "gpiod_ctxless_event_loop() - error in callback after timeout",
	    0, { 8 });

static void ctxless_find_line_good(void)
{
	unsigned int offset;
	char chip[32];
	int rv;

	rv = gpiod_ctxless_find_line("gpio-mockup-C-14", chip,
				     sizeof(chip), &offset);
	TEST_ASSERT_EQ(rv, 1);
	TEST_ASSERT_EQ(offset, 14);
	TEST_ASSERT_STR_EQ(chip, test_chip_name(2));
}
TEST_DEFINE(ctxless_find_line_good,
	    "gpiod_ctxless_find_line() - good",
	    TEST_FLAG_NAMED_LINES, { 8, 16, 16, 8 });

static void ctxless_find_line_truncated(void)
{
	unsigned int offset;
	char chip[6];
	int rv;

	rv = gpiod_ctxless_find_line("gpio-mockup-C-14", chip,
				     sizeof(chip), &offset);
	TEST_ASSERT_EQ(rv, 1);
	TEST_ASSERT_EQ(offset, 14);
	TEST_ASSERT_STR_EQ(chip, "gpioc");
}
TEST_DEFINE(ctxless_find_line_truncated,
	    "gpiod_ctxless_find_line() - chip name truncated",
	    TEST_FLAG_NAMED_LINES, { 8, 16, 16, 8 });

static void ctxless_find_line_not_found(void)
{
	unsigned int offset;
	char chip[32];
	int rv;

	rv = gpiod_ctxless_find_line("nonexistent", chip,
				     sizeof(chip), &offset);
	TEST_ASSERT_EQ(rv, 0);
}
TEST_DEFINE(ctxless_find_line_not_found,
	    "gpiod_ctxless_find_line() - not found",
	    TEST_FLAG_NAMED_LINES, { 8, 16, 16, 8 });
