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

GPIOD_TEST_CASE(rising_edge_active_low, 0, { 8 })
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

	ret = gpiod_line_request_rising_edge_events_flags(line,
		GPIOD_TEST_CONSUMER, GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW);
	g_assert_cmpint(ret, ==, 0);

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);
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

	ret = gpiod_line_get_value(line);
	g_assert_cmpint(ret, ==, 0);

	ev_thread = gpiod_test_start_event_thread(0, 7, 100);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, 1);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);

	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_FALLING_EDGE);
}

GPIOD_TEST_CASE(wait_multiple, 0, { 8 })
{
	g_autoptr(GpiodTestEventThread) ev_thread = NULL;
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_bulk bulk, event_bulk;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	gint ret, i;

	chip = gpiod_chip_open(gpiod_test_chip_path(0));
	g_assert_nonnull(chip);
	gpiod_test_return_if_failed();

	gpiod_line_bulk_init(&bulk);
	for (i = 0; i < 8; i++) {
		line = gpiod_chip_get_line(chip, i);
		g_assert_nonnull(line);
		gpiod_test_return_if_failed();

		gpiod_line_bulk_add(&bulk, line);
	}

	ret = gpiod_line_request_bulk_rising_edge_events(&bulk,
							 GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, 0);

	ev_thread = gpiod_test_start_event_thread(0, 4, 100);

	ret = gpiod_line_event_wait_bulk(&bulk, &ts, &event_bulk);
	g_assert_cmpint(ret, ==, 1);

	g_assert_cmpuint(gpiod_line_bulk_num_lines(&event_bulk), ==, 1);
	line = gpiod_line_bulk_get_line(&event_bulk, 0);
	g_assert_cmpuint(gpiod_line_offset(line), ==, 4);

	ret = gpiod_line_event_read(line, &ev);
	g_assert_cmpint(ret, ==, 0);
	g_assert_cmpint(ev.event_type, ==, GPIOD_LINE_EVENT_RISING_EDGE);
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

	fd = gpiod_line_event_get_fd(line);
	g_assert_cmpint(fd, ==, -1);
	g_assert_cmpint(errno, ==, EPERM);
}

GPIOD_TEST_CASE(request_bulk_fail, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_bulk bulk = GPIOD_LINE_BULK_INITIALIZER;
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

	for (i = 0; i < 8; i++) {
		line = gpiod_chip_get_line(chip, i);
		g_assert_nonnull(line);
		gpiod_test_return_if_failed();
		gpiod_line_bulk_add(&bulk, line);
	}

	ret = gpiod_line_request_bulk_both_edges_events(&bulk,
							GPIOD_TEST_CONSUMER);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EBUSY);
}

GPIOD_TEST_CASE(invalid_fd, 0, { 8 })
{
	g_autoptr(gpiod_chip_struct) chip = NULL;
	struct gpiod_line_bulk bulk = GPIOD_LINE_BULK_INITIALIZER;
	struct gpiod_line_bulk ev_bulk;
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

	fd = gpiod_line_event_get_fd(line);
	close(fd);

	ret = gpiod_line_event_wait(line, &ts);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);

	/*
	 * The single line variant calls gpiod_line_event_wait_bulk() with the
	 * event_bulk argument set to NULL, so test this use case explicitly
	 * as well.
	 */
	gpiod_line_bulk_add(&bulk, line);
	ret = gpiod_line_event_wait_bulk(&bulk, &ts, &ev_bulk);
	g_assert_cmpint(ret, ==, -1);
	g_assert_cmpint(errno, ==, EINVAL);
}
