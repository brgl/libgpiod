/*
 * Simple API test cases for libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "gpiod-test.h"

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
	TEST_ASSERT_EQ(gpiod_errno(), GPIOD_ELINEMAX);
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
	TEST_ASSERT_EQ(gpiod_errno(), GPIOD_ELINEMAX);
}
TEST_DEFINE(simple_set_value_multiple_max_lines,
	    "gpiod_simple_set_value_multiple() exceed max lines",
	    0, { 128 });

struct simple_event_data {
	bool got_event;
};

static int simple_event_cb(int evtype TEST_UNUSED,
			   const struct timespec *ts TEST_UNUSED,
			   void *data)
{
	struct simple_event_data *evdata = data;

	evdata->got_event = true;

	return GPIOD_EVENT_CB_STOP;
}

static void simple_event_loop(void)
{
	struct simple_event_data evdata = { false };
	struct timespec ts = { 1, 0 };
	int status;

	test_set_event(0, 3, TEST_EVENT_ALTERNATING, 100);

	status = gpiod_simple_event_loop(TEST_CONSUMER, test_chip_name(0), 3,
					 false, &ts, simple_event_cb, &evdata);

	TEST_ASSERT_RET_OK(status);
	TEST_ASSERT(evdata.got_event);
}
TEST_DEFINE(simple_event_loop,
	    "gpiod_simple_event_loop() - single event",
	    0, { 8 });
