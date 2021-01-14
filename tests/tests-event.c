// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2019 Bartosz Golaszewski <bgolaszewski@baylibre.com>
 */

#include <errno.h>
#include <unistd.h>

#include "gpiod-test.h"

#define GPIOD_TEST_GROUP "event"

GPIOD_TEST_CASE(rising_edge_good, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_rising_edge_events(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);
}

GPIOD_TEST_CASE(falling_edge_good, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_falling_edge_events(line,
						     GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_FALLING_EDGE);
}

GPIOD_TEST_CASE(rising_edge_ignore_falling, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev[3];
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_rising_edge_events(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);
	ret = gpiod_line_event_read(line, &ev[0]);
	g_assert_cmpint(ret, ==, 0);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);
	ret = gpiod_line_event_read(line, &ev[1]);
	g_assert_cmpint(ret, ==, 0);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);
	ret = gpiod_line_event_read(line, &ev[2]);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev[0].event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);
	g_assert_cmpint(ev[1].event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);
	g_assert_cmpint(ev[2].event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);
}

GPIOD_TEST_CASE(both_edges, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_both_edges_events(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_FALLING_EDGE);
}

GPIOD_TEST_CASE(both_edges_active_low, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_both_edges_events_flags(line,
		GPIOD_TEST_CONSUMER, GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_FALLING_EDGE);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);
}

GPIOD_TEST_CASE(both_edges_bias_disable, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_both_edges_events_flags(line,
		GPIOD_TEST_CONSUMER, GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLED);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_FALLING_EDGE);
}

GPIOD_TEST_CASE(both_edges_bias_pull_down, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_both_edges_events_flags(line,
		GPIOD_TEST_CONSUMER, GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_FALLING_EDGE);
}

GPIOD_TEST_CASE(both_edges_bias_pull_up, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_both_edges_events_flags(line,
		GPIOD_TEST_CONSUMER, GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_FALLING_EDGE);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);
}

GPIOD_TEST_CASE(falling_edge_active_low, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_falling_edge_events_flags(line,
		GPIOD_TEST_CONSUMER, GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_FALLING_EDGE);
}

GPIOD_TEST_CASE(get_value, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	gpiod_test_chip_set_pull(0, 7, 1);

	ret = gpiod_line_request_falling_edge_events(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_line_get_value(line);
	g_assert_cmpint(ret, ==, 1);

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_FALLING_EDGE);
}

GPIOD_TEST_CASE(get_value_active_low, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	gpiod_test_chip_set_pull(0, 7, 1);

	ret = gpiod_line_request_falling_edge_events_flags(line,
		GPIOD_TEST_CONSUMER, GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ret = gpiod_line_get_value(line);
	g_assert_cmpint(ret, ==, 0);

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_FALLING_EDGE);
}

GPIOD_TEST_CASE(get_values, 0, { 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint i, ret, vals[8];

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	bulk = gpiod_line_bulk_new(8);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	for (i = 0; i < 8; i++) {
		line = gpiod_chip_get_line(chip, i);
		g_assert_nonnull(line);
		gpiod_test_return_if_failed();

		gpiod_line_bulk_add_line(bulk, line);
		gpiod_test_chip_set_pull(0, i, 1);
	}

	ret = gpiod_line_request_bulk_rising_edge_events(bulk,
							 GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	memset(vals, 0, sizeof(vals));
	ret = gpiod_line_get_value_bulk(bulk, vals);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(vals[0], ==, 1);
	g_assert_cmpint(vals[1], ==, 1);
	g_assert_cmpint(vals[2], ==, 1);
	g_assert_cmpint(vals[3], ==, 1);
	g_assert_cmpint(vals[4], ==, 1);
	g_assert_cmpint(vals[5], ==, 1);
	g_assert_cmpint(vals[6], ==, 1);
	g_assert_cmpint(vals[7], ==, 1);

	gpiod_test_chip_set_pull(0, 1, 0);
	gpiod_test_chip_set_pull(0, 3, 0);
	gpiod_test_chip_set_pull(0, 4, 0);
	gpiod_test_chip_set_pull(0, 7, 0);

	memset(vals, 0, sizeof(vals));
	ret = gpiod_line_get_value_bulk(bulk, vals);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(vals[0], ==, 1);
	g_assert_cmpint(vals[1], ==, 0);
	g_assert_cmpint(vals[2], ==, 1);
	g_assert_cmpint(vals[3], ==, 0);
	g_assert_cmpint(vals[4], ==, 0);
	g_assert_cmpint(vals[5], ==, 1);
	g_assert_cmpint(vals[6], ==, 1);
	g_assert_cmpint(vals[7], ==, 0);
}

GPIOD_TEST_CASE(get_values_active_low, 0, { 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint i, ret, vals[8];

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	bulk = gpiod_line_bulk_new(8);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	for (i = 0; i < 8; i++) {
		line = gpiod_chip_get_line(chip, i);
		g_assert_nonnull(line);
		gpiod_test_return_if_failed();

		gpiod_line_bulk_add_line(bulk, line);
		gpiod_test_chip_set_pull(0, i, 0);
	}

	ret = gpiod_line_request_bulk_rising_edge_events_flags(bulk,
			GPIOD_TEST_CONSUMER, GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	memset(vals, 0, sizeof(vals));
	ret = gpiod_line_get_value_bulk(bulk, vals);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(vals[0], ==, 1);
	g_assert_cmpint(vals[1], ==, 1);
	g_assert_cmpint(vals[2], ==, 1);
	g_assert_cmpint(vals[3], ==, 1);
	g_assert_cmpint(vals[4], ==, 1);
	g_assert_cmpint(vals[5], ==, 1);
	g_assert_cmpint(vals[6], ==, 1);
	g_assert_cmpint(vals[7], ==, 1);

	gpiod_test_chip_set_pull(0, 1, 1);
	gpiod_test_chip_set_pull(0, 3, 1);
	gpiod_test_chip_set_pull(0, 4, 1);
	gpiod_test_chip_set_pull(0, 7, 1);

	memset(vals, 0, sizeof(vals));
	ret = gpiod_line_get_value_bulk(bulk, vals);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(vals[0], ==, 1);
	g_assert_cmpint(vals[1], ==, 0);
	g_assert_cmpint(vals[2], ==, 1);
	g_assert_cmpint(vals[3], ==, 0);
	g_assert_cmpint(vals[4], ==, 0);
	g_assert_cmpint(vals[5], ==, 1);
	g_assert_cmpint(vals[6], ==, 1);
	g_assert_cmpint(vals[7], ==, 0);
}

GPIOD_TEST_CASE(wait_multiple, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_line_bulk_struct) ev_bulk = NULL;
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret, i;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	bulk = gpiod_line_bulk_new(8);
	ev_bulk = gpiod_line_bulk_new(8);
	g_assert_nonnull(bulk);
	g_assert_nonnull(ev_bulk);
	gpiod_test_return_if_failed();

	for (i = 1; i < 8; i++) {
		line = gpiod_chip_get_line(chip, i);
		g_assert_nonnull(line);
		gpiod_test_return_if_failed();

		gpiod_line_bulk_add_line(bulk, line);
	}

	ret = gpiod_line_request_bulk_rising_edge_events(bulk,
							 GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	ev_thread = gpiod_test_start_event_thread(0, 4, 100);

	ret = gpiod_line_event_wait_bulk(bulk, &ts, ev_bulk);
	g_assert_cmpint(ret, ==, 1);

	g_assert_cmpuint(gpiod_line_bulk_num_lines(ev_bulk), ==, 1);
	line = gpiod_line_bulk_get_line(ev_bulk, 0);
	g_assert_cmpuint(gpiod_line_offset(line), ==, 4);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);
	g_assert_cmpint(ev.offset, ==, 4);
}

GPIOD_TEST_CASE(get_fd_when_values_requested, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret, fd;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 3);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	fd = gpiod_line_event_get_fd(line);
	g_assert_cmpint(fd, ==, -1);
	g_assert_cmpint(errno, ==, EPERM);
}

GPIOD_TEST_CASE(request_bulk_fail, 0, { 8 })
{
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line *line;
	gint ret, i;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_input(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	bulk = gpiod_line_bulk_new(8);
	g_assert_nonnull(bulk);
	gpiod_test_return_if_failed();

	for (i = 0; i < 8; i++) {
		line = gpiod_chip_get_line(chip, i);
		g_assert_nonnull(line);
		gpiod_test_return_if_failed();
		gpiod_line_bulk_add_line(bulk, line);
	}

	ret = gpiod_line_request_bulk_both_edges_events(bulk,
							GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EBUSY);
}

GPIOD_TEST_CASE(invalid_fd, 0, { 8 })
{
	g_autoptr(gpiod_line_bulk_struct) ev_bulk = NULL;
	g_autoptr(gpiod_line_bulk_struct) bulk = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line *line;
	gint ret, fd;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_both_edges_events(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	fd = gpiod_line_event_get_fd(line);
	close(fd);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);

	bulk = gpiod_line_bulk_new(1);
	ev_bulk = gpiod_line_bulk_new(1);
	g_assert_nonnull(bulk);
	g_assert_nonnull(ev_bulk);

	/*
	 * The single line variant calls gpiod_line_event_wait_bulk() with the
	 * event_bulk argument set to NULL, so test this use case explicitly
	 * as well.
	 */
	gpiod_line_bulk_add_line(bulk, line);
	ret = gpiod_line_event_wait_bulk(bulk, &ts, ev_bulk);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);
}

GPIOD_TEST_CASE(read_events_individually, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret;
	guint i;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 7);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_both_edges_events_flags(line,
		GPIOD_TEST_CONSUMER, GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	/* generate multiple events */
	for (i = 0; i < 3; i++) {
		gpiod_test_chip_set_pull(0, 7, i & 1);
		usleep(10000);
	}

	/* read them individually... */
	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);
	if (!ret)
		return;

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_FALLING_EDGE);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);
	if (!ret)
		return;

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);
	if (!ret)
		return;

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_FALLING_EDGE);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 0);
}

GPIOD_TEST_CASE(read_multiple_events, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_event events[5];
	struct timespec ts = { 1, 0 };
	struct gpiod_line *line;
	gint ret;
	guint i;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 4);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_both_edges_events(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	/* generate multiple events */
	for (i = 0; i < 7; i++) {
		gpiod_test_chip_set_pull(0, 4, !(i & 1));
		/*
		 * We sleep for a short period of time here and in other
		 * test cases for multiple events to let the kernel service
		 * each simulated interrupt. Otherwise we'd risk triggering
		 * an interrupt while the previous one is still being
		 * handled.
		 */
		usleep(10000);
	}

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);
	if (!ret)
		return;

	/* read a chunk */
	ret = gpiod_line_event_read_multiple(line, events, 3);
	g_assert_cmpint(ret, ==, 3);

	g_assert_cmpint(events[0].event_type, ==,
			GPIOD_LINE_EVENT_RISING_EDGE);
	g_assert_cmpint(events[1].event_type, ==,
			GPIOD_LINE_EVENT_FALLING_EDGE);
	g_assert_cmpint(events[2].event_type, ==,
			GPIOD_LINE_EVENT_RISING_EDGE);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);
	if (!ret)
		return;

	/*
	 * read the remainder
	 * - note the attempt to read more than are available
	 */
	ret = gpiod_line_event_read_multiple(line, events, 5);
	g_assert_cmpint(ret, ==, 4);

	g_assert_cmpint(events[0].event_type, ==,
			GPIOD_LINE_EVENT_FALLING_EDGE);
	g_assert_cmpint(events[1].event_type, ==,
			GPIOD_LINE_EVENT_RISING_EDGE);
	g_assert_cmpint(events[2].event_type, ==,
			GPIOD_LINE_EVENT_FALLING_EDGE);
	g_assert_cmpint(events[3].event_type, ==,
			GPIOD_LINE_EVENT_RISING_EDGE);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 0);
}

GPIOD_TEST_CASE(read_multiple_events_fd, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_event events[5];
	struct timespec ts = { 1, 0 };
	struct gpiod_line *line;
	gint ret, fd;
	guint i;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	line = gpiod_chip_get_line(chip, 4);
	g_assert_nonnull(line);
	gpiod_test_return_if_failed();

	ret = gpiod_line_request_both_edges_events(line, GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);
	gpiod_test_return_if_failed();

	/* generate multiple events */
	for (i = 0; i < 7; i++) {
		gpiod_test_chip_set_pull(0, 4, !(i & 1));
		usleep(10000);
	}

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);
	if (!ret)
		return;

	fd = gpiod_line_event_get_fd(line);
	g_assert_cmpint(fd, >=, 0);

	/* read a chunk */
	ret = gpiod_line_event_read_fd_multiple(fd, events, 3);
	g_assert_cmpint(ret, ==, 3);

	g_assert_cmpint(events[0].event_type, ==,
			GPIOD_LINE_EVENT_RISING_EDGE);
	g_assert_cmpint(events[1].event_type, ==,
			GPIOD_LINE_EVENT_FALLING_EDGE);
	g_assert_cmpint(events[2].event_type, ==,
			GPIOD_LINE_EVENT_RISING_EDGE);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);
	if (!ret)
		return;

	/*
	 * read the remainder
	 * - note the attempt to read more than are available
	 */
	ret = gpiod_line_event_read_fd_multiple(fd, events, 5);
	g_assert_cmpint(ret, ==, 4);

	g_assert_cmpint(events[0].event_type, ==,
			GPIOD_LINE_EVENT_FALLING_EDGE);
	g_assert_cmpint(events[1].event_type, ==,
			GPIOD_LINE_EVENT_RISING_EDGE);
	g_assert_cmpint(events[2].event_type, ==,
			GPIOD_LINE_EVENT_FALLING_EDGE);
	g_assert_cmpint(events[3].event_type, ==,
			GPIOD_LINE_EVENT_RISING_EDGE);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 0);
}
