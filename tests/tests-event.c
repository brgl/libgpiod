/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 */

/* Test cases for GPIO line events. */

#include "gpiod-test.h"

#include <errno.h>

static void event_rising_edge_good(void)
{
	TEST_CLEANUP(test_close_chip) struct gpiod_chip *chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	int rv;

	chip = gpiod_chip_open(test_chip_path(0));
	TEST_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	TEST_ASSERT_NOT_NULL(line);

	rv = gpiod_line_request_rising_edge_events(line, TEST_CONSUMER);
	TEST_ASSERT_RET_OK(rv);

	test_set_event(0, 7, TEST_EVENT_RISING, 100);

	rv = gpiod_line_event_wait(line, &ts);
	TEST_ASSERT_EQ(rv, 1);

	rv = gpiod_line_event_read(line, &ev);
	TEST_ASSERT_RET_OK(rv);

	TEST_ASSERT_EQ(ev.event_type, GPIOD_LINE_EVENT_RISING_EDGE);
}
TEST_DEFINE(event_rising_edge_good,
	    "events - receive single rising edge event",
	    0, { 8 });

static void event_falling_edge_good(void)
{
	TEST_CLEANUP(test_close_chip) struct gpiod_chip *chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	int rv;

	chip = gpiod_chip_open(test_chip_path(0));
	TEST_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	TEST_ASSERT_NOT_NULL(line);

	rv = gpiod_line_request_falling_edge_events(line, TEST_CONSUMER);
	TEST_ASSERT_RET_OK(rv);

	test_set_event(0, 7, TEST_EVENT_FALLING, 100);

	rv = gpiod_line_event_wait(line, &ts);
	TEST_ASSERT_EQ(rv, 1);

	rv = gpiod_line_event_read(line, &ev);
	TEST_ASSERT_RET_OK(rv);

	TEST_ASSERT_EQ(ev.event_type, GPIOD_LINE_EVENT_FALLING_EDGE);
}
TEST_DEFINE(event_falling_edge_good,
	    "events - receive single falling edge event",
	    0, { 8 });

static void event_rising_edge_ignore_falling(void)
{
	TEST_CLEANUP(test_close_chip) struct gpiod_chip *chip = NULL;
	struct timespec ts = { 0, 300 };
	struct gpiod_line *line;
	int rv;

	chip = gpiod_chip_open(test_chip_path(0));
	TEST_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	TEST_ASSERT_NOT_NULL(line);

	rv = gpiod_line_request_rising_edge_events(line, TEST_CONSUMER);
	TEST_ASSERT_RET_OK(rv);

	test_set_event(0, 7, TEST_EVENT_FALLING, 100);

	rv = gpiod_line_event_wait(line, &ts);
	TEST_ASSERT_EQ(rv, 0);
}
TEST_DEFINE(event_rising_edge_ignore_falling,
	    "events - request rising edge & ignore falling edge events",
	    0, { 8 });

static void event_rising_edge_active_low(void)
{
	TEST_CLEANUP(test_close_chip) struct gpiod_chip *chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	int rv;

	chip = gpiod_chip_open(test_chip_path(0));
	TEST_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	TEST_ASSERT_NOT_NULL(line);

	rv = gpiod_line_request_rising_edge_events_flags(line, TEST_CONSUMER,
					GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW);
	TEST_ASSERT_RET_OK(rv);

	test_set_event(0, 7, TEST_EVENT_RISING, 100);

	rv = gpiod_line_event_wait(line, &ts);
	TEST_ASSERT_EQ(rv, 1);

	rv = gpiod_line_event_read(line, &ev);
	TEST_ASSERT_RET_OK(rv);

	TEST_ASSERT_EQ(ev.event_type, GPIOD_LINE_EVENT_RISING_EDGE);
}
TEST_DEFINE(event_rising_edge_active_low,
	    "events - single rising edge event with low active state",
	    0, { 8 });

static void event_get_value(void)
{
	TEST_CLEANUP(test_close_chip) struct gpiod_chip *chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	int rv;

	chip = gpiod_chip_open(test_chip_path(0));
	TEST_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	TEST_ASSERT_NOT_NULL(line);

	rv = gpiod_line_request_rising_edge_events(line, TEST_CONSUMER);
	TEST_ASSERT_RET_OK(rv);

	rv = gpiod_line_get_value(line);
	TEST_ASSERT_EQ(rv, 0);

	test_set_event(0, 7, TEST_EVENT_RISING, 100);

	rv = gpiod_line_event_wait(line, &ts);
	TEST_ASSERT_EQ(rv, 1);

	rv = gpiod_line_event_read(line, &ev);
	TEST_ASSERT_RET_OK(rv);

	TEST_ASSERT_EQ(ev.event_type, GPIOD_LINE_EVENT_RISING_EDGE);

	rv = gpiod_line_get_value(line);
	TEST_ASSERT_EQ(rv, 1);
}
TEST_DEFINE(event_get_value,
	    "events - mixing events and gpiod_line_get_value()",
	    0, { 8 });

static void event_wait_multiple(void)
{
	TEST_CLEANUP(test_close_chip) struct gpiod_chip *chip = NULL;
	struct gpiod_line_bulk bulk, event_bulk;
	struct timespec ts = { 1, 0 };
	struct gpiod_line *line;
	int rv, i;

	chip = gpiod_chip_open(test_chip_path(0));
	TEST_ASSERT_NOT_NULL(chip);

	gpiod_line_bulk_init(&bulk);

	for (i = 0; i < 8; i++) {
		line = gpiod_chip_get_line(chip, i);
		TEST_ASSERT_NOT_NULL(line);

		gpiod_line_bulk_add(&bulk, line);
	}

	rv = gpiod_line_request_bulk_both_edges_events(&bulk, TEST_CONSUMER);
	TEST_ASSERT_RET_OK(rv);

	test_set_event(0, 4, TEST_EVENT_RISING, 100);

	rv = gpiod_line_event_wait_bulk(&bulk, &ts, &event_bulk);
	TEST_ASSERT_EQ(rv, 1);

	TEST_ASSERT_EQ(gpiod_line_bulk_num_lines(&event_bulk), 1);
	line = gpiod_line_bulk_get_line(&event_bulk, 0);
	TEST_ASSERT_EQ(gpiod_line_offset(line), 4);
}
TEST_DEFINE(event_wait_multiple,
	    "events - wait for events on multiple lines",
	    0, { 8 });

static void event_get_fd_when_values_requested(void)
{
	TEST_CLEANUP(test_close_chip) struct gpiod_chip *chip = NULL;
	struct gpiod_line *line;
	int rv, fd;

	chip = gpiod_chip_open(test_chip_path(0));
	TEST_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 3);
	TEST_ASSERT_NOT_NULL(line);

	rv = gpiod_line_request_input(line, TEST_CONSUMER);
	TEST_ASSERT_RET_OK(rv);

	fd = gpiod_line_event_get_fd(line);
	TEST_ASSERT_EQ(fd, -1);
	TEST_ASSERT_ERRNO_IS(EPERM);
}
TEST_DEFINE(event_get_fd_when_values_requested,
	    "events - gpiod_line_event_get_fd(): line requested for values",
	    0, { 8 });

static void event_request_bulk_fail(void)
{
	TEST_CLEANUP(test_close_chip) struct gpiod_chip *chip = NULL;
	struct gpiod_line_bulk bulk = GPIOD_LINE_BULK_INITIALIZER;
	struct gpiod_line *line;
	int rv, i;

	chip = gpiod_chip_open(test_chip_path(0));
	TEST_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 5);
	TEST_ASSERT_NOT_NULL(line);

	rv = gpiod_line_request_input(line, TEST_CONSUMER);
	TEST_ASSERT_RET_OK(rv);

	for (i = 0; i < 8; i++) {
		line = gpiod_chip_get_line(chip, i);
		TEST_ASSERT_NOT_NULL(line);
		gpiod_line_bulk_add(&bulk, line);
	}

	rv = gpiod_line_request_bulk_both_edges_events(&bulk, TEST_CONSUMER);
	TEST_ASSERT_EQ(rv, -1);
	TEST_ASSERT_ERRNO_IS(EBUSY);
}
TEST_DEFINE(event_request_bulk_fail,
	    "events - failed bulk request (test reversed release)",
	    0, { 8 });
