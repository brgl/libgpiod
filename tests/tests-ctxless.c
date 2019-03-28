// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

/* Test cases for the high-level API. */

#include <errno.h>

#include "gpiod-test.h"

static void ctxless_get_value(void)
{
	int rv;

	rv = gpiod_ctxless_get_value(test_chip_name(0), 3,
				     false, TEST_CONSUMER);
	TEST_ASSERT_EQ(rv, 0);

	test_debugfs_set_value(0, 3, 1);

	rv = gpiod_ctxless_get_value(test_chip_name(0), 3,
				     false, TEST_CONSUMER);
	TEST_ASSERT_EQ(rv, 1);
}
TEST_DEFINE(ctxless_get_value,
	    "ctxless get value - single line",
	    0, { 8 });

static void ctxless_set_value_check(void *data)
{
	int *val = data;

	*val = test_debugfs_get_value(0, 3);
}

static void ctxless_set_value(void)
{
	int rv, val;

	TEST_ASSERT_EQ(test_debugfs_get_value(0, 3), 0);

	rv = gpiod_ctxless_set_value(test_chip_name(0), 3, 1,
				     false, TEST_CONSUMER,
				     ctxless_set_value_check, &val);
	TEST_ASSERT_RET_OK(rv);

	TEST_ASSERT_EQ(val, 1);
	TEST_ASSERT_EQ(test_debugfs_get_value(0, 3), 0);
}
TEST_DEFINE(ctxless_set_value,
	    "ctxless set value - single line",
	    0, { 8 });

static const unsigned int ctxless_set_value_multiple_offsets[] = {
	0, 1, 2, 3, 4, 5, 6, 12, 13, 15
};

static const int ctxless_set_value_multiple_values[] = {
	1, 1, 1, 0, 0, 1, 0, 1, 0, 0
};

static void ctxless_set_value_multiple_check(void *data)
{
	bool *vals_correct = data;
	unsigned int offset, i;
	int val, exp;

	for (i = 0;
	     i < TEST_ARRAY_SIZE(ctxless_set_value_multiple_values);
	     i++) {
		offset = ctxless_set_value_multiple_offsets[i];
		exp = ctxless_set_value_multiple_values[i];
		val = test_debugfs_get_value(0, offset);

		if (val != exp) {
			*vals_correct = false;
			break;
		}
	}
}

static void ctxless_set_get_value_multiple(void)
{
	bool vals_correct = true;
	int values[10], rv;
	unsigned int i;

	for (i = 0;
	     i < TEST_ARRAY_SIZE(ctxless_set_value_multiple_offsets);
	     i++) {
		TEST_ASSERT_EQ(test_debugfs_get_value(0,
				ctxless_set_value_multiple_offsets[i]), 0);
	}

	for (i = 0;
	     i < TEST_ARRAY_SIZE(ctxless_set_value_multiple_values);
	     i++) {
		values[i] = ctxless_set_value_multiple_values[i];
	}

	rv = gpiod_ctxless_set_value_multiple(test_chip_name(0),
					ctxless_set_value_multiple_offsets,
					values, 10, false, TEST_CONSUMER,
					ctxless_set_value_multiple_check,
					&vals_correct);
	TEST_ASSERT_RET_OK(rv);
	TEST_ASSERT(vals_correct);
}
TEST_DEFINE(ctxless_set_get_value_multiple,
	    "ctxless set/get value - multiple lines",
	    0, { 16 });

static void ctxless_get_value_multiple_max_lines(void)
{
	unsigned int offsets[GPIOD_LINE_BULK_MAX_LINES + 1];
	int values[GPIOD_LINE_BULK_MAX_LINES + 1], rv;

	rv = gpiod_ctxless_get_value_multiple(test_chip_name(0), offsets,
					      values,
					      GPIOD_LINE_BULK_MAX_LINES + 1,
					      false, TEST_CONSUMER);
	TEST_ASSERT_NOTEQ(rv, 0);
	TEST_ASSERT_ERRNO_IS(EINVAL);
}
TEST_DEFINE(ctxless_get_value_multiple_max_lines,
	    "gpiod_ctxless_get_value_multiple() exceed max lines",
	    0, { 128 });

static void ctxless_set_value_multiple_max_lines(void)
{
	unsigned int offsets[GPIOD_LINE_BULK_MAX_LINES + 1];
	int values[GPIOD_LINE_BULK_MAX_LINES + 1], rv;

	rv = gpiod_ctxless_set_value_multiple(test_chip_name(0), offsets,
					      values,
					      GPIOD_LINE_BULK_MAX_LINES + 1,
					      false, TEST_CONSUMER,
					      NULL, NULL);
	TEST_ASSERT_NOTEQ(rv, 0);
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

static void ctxless_event_monitor(void)
{
	struct ctxless_event_data evdata = { false, false, 0, 0 };
	struct timespec ts = { 1, 0 };
	int rv;

	test_set_event(0, 3, 100);

	rv = gpiod_ctxless_event_monitor(test_chip_name(0),
					 GPIOD_CTXLESS_EVENT_BOTH_EDGES,
					 3, false, TEST_CONSUMER, &ts,
					 NULL, ctxless_event_cb, &evdata);

	TEST_ASSERT_RET_OK(rv);
	TEST_ASSERT(evdata.got_rising_edge);
	TEST_ASSERT(evdata.got_falling_edge);
	TEST_ASSERT_EQ(evdata.count, 2);
	TEST_ASSERT_EQ(evdata.offset, 3);
}
TEST_DEFINE(ctxless_event_monitor,
	    "gpiod_ctxless_event_monitor() - single event",
	    0, { 8 });

static void ctxless_event_monitor_single_event_type(void)
{
	struct ctxless_event_data evdata = { false, false, 0, 0 };
	struct timespec ts = { 1, 0 };
	int rv;

	test_set_event(0, 3, 100);

	rv = gpiod_ctxless_event_monitor(test_chip_name(0),
					 GPIOD_CTXLESS_EVENT_FALLING_EDGE,
					 3, false, TEST_CONSUMER, &ts,
					 NULL, ctxless_event_cb, &evdata);

	TEST_ASSERT_RET_OK(rv);
	TEST_ASSERT(evdata.got_falling_edge);
	TEST_ASSERT_FALSE(evdata.got_rising_edge);
	TEST_ASSERT_EQ(evdata.count, 2);
	TEST_ASSERT_EQ(evdata.offset, 3);
}
TEST_DEFINE(ctxless_event_monitor_single_event_type,
	    "gpiod_ctxless_event_monitor() - specify event type",
	    0, { 8 });

static void ctxless_event_monitor_multiple(void)
{
	struct ctxless_event_data evdata = { false, false, 0, 0 };
	struct timespec ts = { 1, 0 };
	unsigned int offsets[4];
	int rv;

	offsets[0] = 2;
	offsets[1] = 3;
	offsets[2] = 5;
	offsets[3] = 6;

	test_set_event(0, 3, 100);

	rv = gpiod_ctxless_event_monitor_multiple(
					test_chip_name(0),
					GPIOD_CTXLESS_EVENT_BOTH_EDGES,
					offsets, 4, false, TEST_CONSUMER,
					&ts, NULL, ctxless_event_cb, &evdata);

	TEST_ASSERT_RET_OK(rv);
	TEST_ASSERT(evdata.got_rising_edge);
	TEST_ASSERT(evdata.got_falling_edge);
	TEST_ASSERT_EQ(evdata.count, 2);
	TEST_ASSERT_EQ(evdata.offset, 3);
}
TEST_DEFINE(ctxless_event_monitor_multiple,
	    "gpiod_ctxless_event_monitor_multiple() - single event",
	    0, { 8 });

static int error_event_cb(int evtype TEST_UNUSED,
			  unsigned int offset TEST_UNUSED,
			  const struct timespec *ts TEST_UNUSED,
			  void *data TEST_UNUSED)
{
	errno = ENOTBLK;

	return GPIOD_CTXLESS_EVENT_CB_RET_ERR;
}

static void ctxless_event_monitor_indicate_error(void)
{
	struct timespec ts = { 1, 0 };
	int rv;

	test_set_event(0, 3, 100);

	rv = gpiod_ctxless_event_monitor(test_chip_name(0),
					 GPIOD_CTXLESS_EVENT_BOTH_EDGES,
					 3, false, TEST_CONSUMER, &ts,
					 NULL, error_event_cb, NULL);

	TEST_ASSERT_EQ(rv, -1);
	TEST_ASSERT_ERRNO_IS(ENOTBLK);
}
TEST_DEFINE(ctxless_event_monitor_indicate_error,
	    "gpiod_ctxless_event_monitor() - error in callback",
	    0, { 8 });

static void ctxless_event_monitor_indicate_error_timeout(void)
{
	struct timespec ts = { 0, 100000 };
	int rv;

	rv = gpiod_ctxless_event_monitor(test_chip_name(0),
					 GPIOD_CTXLESS_EVENT_BOTH_EDGES,
					 3, false, TEST_CONSUMER, &ts,
					 NULL, error_event_cb, NULL);

	TEST_ASSERT_EQ(rv, -1);
	TEST_ASSERT_ERRNO_IS(ENOTBLK);
}
TEST_DEFINE(ctxless_event_monitor_indicate_error_timeout,
	    "gpiod_ctxless_event_monitor() - error in callback after timeout",
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
