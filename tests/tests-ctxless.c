// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>

#include "gpiod-test.h"

#define GPIOD_TEST_GROUP "ctxless"

GPIOD_TEST_CASE(get_value, 0, { 8 })
{
	gint ret;

	ret = gpiod_ctxless_get_value(gpiod_test_chip_name(0), 3,
				      false, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);

	gpiod_test_chip_set_pull(0, 3, 1);

	ret = gpiod_ctxless_get_value(gpiod_test_chip_name(0), 3,
				      false, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 1);
}

GPIOD_TEST_CASE(get_value_ext, 0, { 8 })
{
	gint ret;

	ret = gpiod_ctxless_get_value_ext(gpiod_test_chip_name(0), 3,
				false, GPIOD_TEST_CONSUMER,
				GPIOD_CTXLESS_FLAG_BIAS_PULL_DOWN);
	g_assert_cmpint(ret, ==, 0);

	ret = gpiod_ctxless_get_value_ext(gpiod_test_chip_name(0), 3,
				false, GPIOD_TEST_CONSUMER,
				GPIOD_CTXLESS_FLAG_BIAS_PULL_UP);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_ctxless_get_value_ext(gpiod_test_chip_name(0), 3,
				true, GPIOD_TEST_CONSUMER
				, GPIOD_CTXLESS_FLAG_BIAS_PULL_DOWN);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_ctxless_get_value_ext(gpiod_test_chip_name(0), 3,
				true, GPIOD_TEST_CONSUMER,
				GPIOD_CTXLESS_FLAG_BIAS_PULL_UP);
	g_assert_cmpint(ret, ==, 0);
}

static void set_value_check_hi(gpointer data G_GNUC_UNUSED)
{
	g_assert_cmpint(gpiod_test_chip_get_value(0, 3), ==, 1);
}

static void set_value_check_lo(gpointer data G_GNUC_UNUSED)
{
	g_assert_cmpint(gpiod_test_chip_get_value(0, 3), ==, 0);
}

GPIOD_TEST_CASE(set_value, 0, { 8 })
{
	gint ret;

	gpiod_test_chip_set_pull(0, 3, 0);

	ret = gpiod_ctxless_set_value(gpiod_test_chip_name(0), 3, 1,
				      false, GPIOD_TEST_CONSUMER,
				      set_value_check_hi, NULL);
	gpiod_test_return_if_failed();
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(gpiod_test_chip_get_value(0, 3), ==, 0);
}

GPIOD_TEST_CASE(set_value_ext, 0, { 8 })
{
	gint ret;

	gpiod_test_chip_set_pull(0, 3, 0);

	ret = gpiod_ctxless_set_value_ext(gpiod_test_chip_name(0), 3, 1,
			false, GPIOD_TEST_CONSUMER,
			set_value_check_hi, NULL, 0);
	gpiod_test_return_if_failed();
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 3), ==, 0);

	/* test drive flags by checking that sets are caught by emulation */
	ret = gpiod_ctxless_set_value_ext(gpiod_test_chip_name(0), 3, 1,
			false, GPIOD_TEST_CONSUMER, set_value_check_lo,
			NULL, GPIOD_CTXLESS_FLAG_OPEN_DRAIN);
	gpiod_test_return_if_failed();
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 3), ==, 0);

	gpiod_test_chip_set_pull(0, 3, 1);
	ret = gpiod_ctxless_set_value_ext(gpiod_test_chip_name(0), 3, 0,
			false, GPIOD_TEST_CONSUMER, set_value_check_hi,
			NULL, GPIOD_CTXLESS_FLAG_OPEN_SOURCE);
	gpiod_test_return_if_failed();
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(gpiod_test_chip_get_value(0, 3), ==, 1);
}

static const guint get_value_multiple_offsets[] = {
	1, 3, 4, 5, 6, 7, 8, 9, 13, 14
};

static const gint get_value_multiple_expected[] = {
	1, 1, 1, 0, 0, 0, 1, 0, 1, 1
};

GPIOD_TEST_CASE(get_value_multiple, 0, { 16 })
{
	gint ret, values[10];
	guint i;

	for (i = 0; i < G_N_ELEMENTS(get_value_multiple_offsets); i++)
		gpiod_test_chip_set_pull(0, get_value_multiple_offsets[i],
					 get_value_multiple_expected[i]);

	ret = gpiod_ctxless_get_value_multiple(gpiod_test_chip_name(0),
					       get_value_multiple_offsets,
					       values, 10, false,
					       GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);

	for (i = 0; i < G_N_ELEMENTS(get_value_multiple_offsets); i++)
		g_assert_cmpint(values[i], ==, get_value_multiple_expected[i]);
}

static const guint set_value_multiple_offsets[] = {
	0, 1, 2, 3, 4, 5, 6, 12, 13, 15
};

static const gint set_value_multiple_values[] = {
	1, 1, 1, 0, 0, 1, 0, 1, 0, 0
};

static void set_value_multiple_check(gpointer data G_GNUC_UNUSED)
{
	guint i, offset;
	gint val, exp;

	for (i = 0; i < G_N_ELEMENTS(set_value_multiple_values); i++) {
		offset = set_value_multiple_offsets[i];
		exp = set_value_multiple_values[i];
		val = gpiod_test_chip_get_value(0, offset);

		g_assert_cmpint(val, ==, exp);
	}
}

GPIOD_TEST_CASE(set_value_multiple, 0, { 16 })
{
	gint values[10], ret;
	guint i;

	for (i = 0; i < G_N_ELEMENTS(set_value_multiple_offsets); i++)
		values[i] = set_value_multiple_values[i];

	ret = gpiod_ctxless_set_value_multiple(gpiod_test_chip_name(0),
			set_value_multiple_offsets, values, 10, false,
			GPIOD_TEST_CONSUMER, set_value_multiple_check, NULL);
	gpiod_test_return_if_failed();
	g_assert_cmpint(ret, ==, 0);
}

GPIOD_TEST_CASE(get_value_multiple_max_lines, 0, { 128 })
{
	gint values[GPIOD_LINE_BULK_MAX_LINES + 1], ret;
	guint offsets[GPIOD_LINE_BULK_MAX_LINES + 1];

	ret = gpiod_ctxless_get_value_multiple(gpiod_test_chip_name(0),
					       offsets, values,
					       GPIOD_LINE_BULK_MAX_LINES + 1,
					       false, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);
}

GPIOD_TEST_CASE(set_value_multiple_max_lines, 0, { 128 })
{
	gint values[GPIOD_LINE_BULK_MAX_LINES + 1], ret;
	guint offsets[GPIOD_LINE_BULK_MAX_LINES + 1];

	ret = gpiod_ctxless_set_value_multiple(gpiod_test_chip_name(0),
				offsets, values, GPIOD_LINE_BULK_MAX_LINES + 1,
				false, GPIOD_TEST_CONSUMER, NULL, NULL);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);
}

struct ctxless_event_data {
	gboolean got_rising_edge;
	gboolean got_falling_edge;
	guint offset;
	guint count;
};

static int ctxless_event_cb(gint evtype, guint offset,
			    const struct timespec *ts G_GNUC_UNUSED,
			    gpointer data)
{
	struct ctxless_event_data *evdata = data;

	if (evtype == GPIOD_CTXLESS_EVENT_CB_RISING_EDGE)
		evdata->got_rising_edge = TRUE;
	else if (evtype == GPIOD_CTXLESS_EVENT_CB_FALLING_EDGE)
		evdata->got_falling_edge = TRUE;

	evdata->offset = offset;

	return ++evdata->count == 2 ? GPIOD_CTXLESS_EVENT_CB_RET_STOP
				    : GPIOD_CTXLESS_EVENT_CB_RET_OK;
}

GPIOD_TEST_CASE(event_monitor, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	struct ctxless_event_data evdata = { false, false, 0, 0 };
	struct timespec ts = { 1, 0 };
	gint ret;

	ev_thread = gpiod_test_start_event_thread(0, 3, 100);

	ret = gpiod_ctxless_event_monitor(gpiod_test_chip_name(0),
					  GPIOD_CTXLESS_EVENT_BOTH_EDGES,
					  3, false, GPIOD_TEST_CONSUMER, &ts,
					  NULL, ctxless_event_cb, &evdata);
	g_assert_cmpint(ret, ==, 0);
	g_assert_true(evdata.got_rising_edge);
	g_assert_true(evdata.got_falling_edge);
	g_assert_cmpuint(evdata.count, ==, 2);
	g_assert_cmpuint(evdata.offset, ==, 3);
}

GPIOD_TEST_CASE(event_monitor_single_event_type, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	struct ctxless_event_data evdata = { false, false, 0, 0 };
	struct timespec ts = { 1, 0 };
	gint ret;

	ev_thread = gpiod_test_start_event_thread(0, 3, 100);

	ret = gpiod_ctxless_event_monitor(gpiod_test_chip_name(0),
					  GPIOD_CTXLESS_EVENT_FALLING_EDGE,
					  3, false, GPIOD_TEST_CONSUMER, &ts,
					  NULL, ctxless_event_cb, &evdata);
	g_assert_cmpint(ret, ==, 0);
	g_assert_false(evdata.got_rising_edge);
	g_assert_true(evdata.got_falling_edge);
	g_assert_cmpuint(evdata.count, ==, 2);
	g_assert_cmpuint(evdata.offset, ==, 3);
}

GPIOD_TEST_CASE(event_monitor_multiple, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	struct ctxless_event_data evdata = { false, false, 0, 0 };
	struct timespec ts = { 1, 0 };
	guint offsets[4];
	gint ret;

	offsets[0] = 2;
	offsets[1] = 3;
	offsets[2] = 5;
	offsets[3] = 6;

	ev_thread = gpiod_test_start_event_thread(0, 3, 100);

	ret = gpiod_ctxless_event_monitor_multiple(gpiod_test_chip_name(0),
		GPIOD_CTXLESS_EVENT_BOTH_EDGES, offsets, 4, false,
		GPIOD_TEST_CONSUMER, &ts, NULL, ctxless_event_cb, &evdata);
	g_assert_cmpint(ret, ==, 0);
	g_assert_true(evdata.got_rising_edge);
	g_assert_true(evdata.got_falling_edge);
	g_assert_cmpuint(evdata.count, ==, 2);
	g_assert_cmpuint(evdata.offset, ==, 3);
}

static int error_event_cb(gint evtype G_GNUC_UNUSED,
			  guint offset G_GNUC_UNUSED,
			  const struct timespec *ts G_GNUC_UNUSED,
			  gpointer data G_GNUC_UNUSED)
{
	errno = ENOTBLK;

	return GPIOD_CTXLESS_EVENT_CB_RET_ERR;
}

GPIOD_TEST_CASE(event_monitor_indicate_error, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	struct timespec ts = { 1, 0 };
	gint ret;

	ev_thread = gpiod_test_start_event_thread(0, 3, 100);

	ret = gpiod_ctxless_event_monitor(gpiod_test_chip_name(0),
					  GPIOD_CTXLESS_EVENT_BOTH_EDGES,
					  3, false, GPIOD_TEST_CONSUMER, &ts,
					  NULL, error_event_cb, NULL);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, ENOTBLK);
}

static int error_event_cb_timeout(gint evtype,
				  guint offset G_GNUC_UNUSED,
				  const struct timespec *ts G_GNUC_UNUSED,
				  gpointer data G_GNUC_UNUSED)
{
	errno = ENOTBLK;

	g_assert_cmpint(evtype, ==, GPIOD_CTXLESS_EVENT_CB_TIMEOUT);

	return GPIOD_CTXLESS_EVENT_CB_RET_ERR;
}

GPIOD_TEST_CASE(event_monitor_indicate_error_timeout, 0, { 8 })
{
	struct timespec ts = { 0, 100000 };
	gint ret;

	ret = gpiod_ctxless_event_monitor(gpiod_test_chip_name(0),
					  GPIOD_CTXLESS_EVENT_BOTH_EDGES,
					  3, false, GPIOD_TEST_CONSUMER, &ts,
					  NULL, error_event_cb_timeout, NULL);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, ENOTBLK);
}

GPIOD_TEST_CASE(find_line, GPIOD_TEST_FLAG_NAMED_LINES, { 8, 16, 16, 8 })
{
	gchar chip[32];
	guint offset;
	gint ret;

	ret = gpiod_ctxless_find_line("gpio-mockup-C-14", chip,
				      sizeof(chip), &offset);
	g_assert_cmpint(ret, ==, 1);
	g_assert_cmpuint(offset, ==, 14);
	g_assert_cmpstr(chip, ==, gpiod_test_chip_name(2));
}

GPIOD_TEST_CASE(find_line_truncated,
		GPIOD_TEST_FLAG_NAMED_LINES, { 8, 16, 16, 8 })
{
	gchar chip[6];
	guint offset;
	gint ret;

	ret = gpiod_ctxless_find_line("gpio-mockup-C-14", chip,
				      sizeof(chip), &offset);
	g_assert_cmpint(ret, ==, 1);
	g_assert_cmpuint(offset, ==, 14);
	g_assert_cmpstr(chip, ==, "gpioc");
}

GPIOD_TEST_CASE(find_line_not_found,
		GPIOD_TEST_FLAG_NAMED_LINES, { 8, 16, 16, 8 })
{
	gchar chip[32];
	guint offset;
	gint ret;

	ret = gpiod_ctxless_find_line("nonexistent", chip,
				      sizeof(chip), &offset);
	g_assert_cmpint(ret, ==, 0);
}
