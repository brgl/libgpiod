/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

/* Test cases for the simple API. */

#include "gpiod-test.h"

#include <errno.h>

static void simple_set_get_value(void)
{
	int ret;

	ret = gpiod_simple_get_value(TEST_CONSUMER,
				     test_chip_name(0), 3, false);
	TEST_ASSERT_EQ(ret, 0);

	ret = gpiod_simple_set_value(TEST_CONSUMER, test_chip_name(0),
				     3, 1, false, NULL, NULL);
	TEST_ASSERT_RET_OK(ret);

	ret = gpiod_simple_get_value(TEST_CONSUMER,
				     test_chip_name(0), 3, false);
	TEST_ASSERT_EQ(ret, 1);
}
TEST_DEFINE(simple_set_get_value,
	    "simple set/get value - single line",
	    0, { 8 });

static void simple_set_get_value_multiple(void)
{
	unsigned int offsets[] = { 0, 1, 2, 3, 4, 5, 6, 12, 13, 15 };
	int values[10], ret;

	ret = gpiod_simple_get_value_multiple(TEST_CONSUMER, test_chip_name(0),
					      offsets, values, 10, false);
	TEST_ASSERT_RET_OK(ret);

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

	ret = gpiod_simple_set_value_multiple(TEST_CONSUMER, test_chip_name(0),
					      offsets, values, 10, false,
					      NULL, NULL);
	TEST_ASSERT_RET_OK(ret);

	ret = gpiod_simple_get_value_multiple(TEST_CONSUMER, test_chip_name(0),
					      offsets, values, 10, false);
	TEST_ASSERT_RET_OK(ret);

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
TEST_DEFINE(simple_set_get_value_multiple,
	    "simple set/get value - multiple lines",
	    0, { 16 });

static void simple_get_value_multiple_max_lines(void)
{
	unsigned int offsets[GPIOD_REQUEST_MAX_LINES + 1];
	int values[GPIOD_REQUEST_MAX_LINES + 1], ret;

	ret = gpiod_simple_get_value_multiple(TEST_CONSUMER, test_chip_name(0),
					      offsets, values,
					      GPIOD_REQUEST_MAX_LINES + 1,
					      false);
	TEST_ASSERT_NOTEQ(ret, 0);
	TEST_ASSERT_EQ(errno, EINVAL);
}
TEST_DEFINE(simple_get_value_multiple_max_lines,
	    "gpiod_simple_get_value_multiple() exceed max lines",
	    0, { 128 });

static void simple_set_value_multiple_max_lines(void)
{
	unsigned int offsets[GPIOD_REQUEST_MAX_LINES + 1];
	int values[GPIOD_REQUEST_MAX_LINES + 1], ret;

	ret = gpiod_simple_set_value_multiple(TEST_CONSUMER, test_chip_name(0),
					      offsets, values,
					      GPIOD_REQUEST_MAX_LINES + 1,
					      false, NULL, NULL);
	TEST_ASSERT_NOTEQ(ret, 0);
	TEST_ASSERT_EQ(errno, EINVAL);
}
TEST_DEFINE(simple_set_value_multiple_max_lines,
	    "gpiod_simple_set_value_multiple() exceed max lines",
	    0, { 128 });

struct simple_event_data {
	bool got_rising_edge;
	bool got_falling_edge;
	unsigned int offset;
	unsigned int count;
};

static int simple_event_cb(int evtype, unsigned int offset,
			   const struct timespec *ts TEST_UNUSED, void *data)
{
	struct simple_event_data *evdata = data;

	if (evtype == GPIOD_EVENT_CB_RISING_EDGE)
		evdata->got_rising_edge = true;
	else if (evtype == GPIOD_EVENT_CB_FALLING_EDGE)
		evdata->got_falling_edge = true;

	evdata->offset = offset;

	return ++evdata->count == 2 ? GPIOD_EVENT_CB_STOP : GPIOD_EVENT_CB_OK;
}

static void simple_event_loop(void)
{
	struct simple_event_data evdata = { false, false, 0, 0 };
	struct timespec ts = { 1, 0 };
	int status;

	test_set_event(0, 3, TEST_EVENT_ALTERNATING, 100);

	status = gpiod_simple_event_loop(TEST_CONSUMER, test_chip_name(0), 3,
					 false, &ts, NULL, simple_event_cb,
					 &evdata);

	TEST_ASSERT_RET_OK(status);
	TEST_ASSERT(evdata.got_rising_edge);
	TEST_ASSERT(evdata.got_falling_edge);
	TEST_ASSERT_EQ(evdata.count, 2);
	TEST_ASSERT_EQ(evdata.offset, 3);
}
TEST_DEFINE(simple_event_loop,
	    "gpiod_simple_event_loop() - single event",
	    0, { 8 });

static void simple_event_loop_multiple(void)
{
	struct simple_event_data evdata = { false, false, 0, 0 };
	struct timespec ts = { 1, 0 };
	unsigned int offsets[4];
	int status;

	offsets[0] = 2;
	offsets[1] = 3;
	offsets[2] = 5;
	offsets[3] = 6;

	test_set_event(0, 3, TEST_EVENT_ALTERNATING, 100);

	status = gpiod_simple_event_loop_multiple(TEST_CONSUMER,
						  test_chip_name(0), offsets,
						  4, false, &ts, NULL,
						  simple_event_cb, &evdata);

	TEST_ASSERT_RET_OK(status);
	TEST_ASSERT(evdata.got_rising_edge);
	TEST_ASSERT(evdata.got_falling_edge);
	TEST_ASSERT_EQ(evdata.count, 2);
	TEST_ASSERT_EQ(evdata.offset, 3);
}
TEST_DEFINE(simple_event_loop_multiple,
	    "gpiod_simple_event_loop_multiple() - single event",
	    0, { 8 });
