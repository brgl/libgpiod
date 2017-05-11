/*
 * Line event test cases for libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "gpiod-unit.h"

static void event_rising_edge_good(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	int status;

	chip = gpiod_chip_open(gu_chip_path(0));
	GU_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	GU_ASSERT_NOT_NULL(line);

	status = gpiod_line_event_request_rising(line, "gpiod-unit", false);
	GU_ASSERT_RET_OK(status);

	gu_set_event(0, 7, GU_EVENT_RISING, 100);

	status = gpiod_line_event_wait(line, &ts);
	GU_ASSERT_EQ(status, 1);

	status = gpiod_line_event_read(line, &ev);
	GU_ASSERT_RET_OK(status);

	GU_ASSERT_EQ(ev.event_type, GPIOD_EVENT_RISING_EDGE);
}
GU_DEFINE_TEST(event_rising_edge_good,
	       "events - receive single rising edge event",
	       GU_LINES_UNNAMED, { 8 });

static void event_falling_edge_good(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	int status;

	chip = gpiod_chip_open(gu_chip_path(0));
	GU_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	GU_ASSERT_NOT_NULL(line);

	status = gpiod_line_event_request_falling(line, "gpiod-unit", false);
	GU_ASSERT_RET_OK(status);

	gu_set_event(0, 7, GU_EVENT_FALLING, 100);

	status = gpiod_line_event_wait(line, &ts);
	GU_ASSERT_EQ(status, 1);

	status = gpiod_line_event_read(line, &ev);
	GU_ASSERT_RET_OK(status);

	GU_ASSERT_EQ(ev.event_type, GPIOD_EVENT_FALLING_EDGE);
}
GU_DEFINE_TEST(event_falling_edge_good,
	       "events - receive single falling edge event",
	       GU_LINES_UNNAMED, { 8 });

static void event_rising_edge_ignore_falling(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;
	struct timespec ts = { 0, 300 };
	struct gpiod_line *line;
	int status;

	chip = gpiod_chip_open(gu_chip_path(0));
	GU_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	GU_ASSERT_NOT_NULL(line);

	status = gpiod_line_event_request_rising(line, "gpiod-unit", false);
	GU_ASSERT_RET_OK(status);

	gu_set_event(0, 7, GU_EVENT_FALLING, 100);

	status = gpiod_line_event_wait(line, &ts);
	GU_ASSERT_EQ(status, 0);
}
GU_DEFINE_TEST(event_rising_edge_ignore_falling,
	       "events - request rising edge & ignore falling edge events",
	       GU_LINES_UNNAMED, { 8 });

static void event_rising_edge_active_low(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	int status;

	chip = gpiod_chip_open(gu_chip_path(0));
	GU_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	GU_ASSERT_NOT_NULL(line);

	status = gpiod_line_event_request_rising(line, "gpiod-unit", true);
	GU_ASSERT_RET_OK(status);

	gu_set_event(0, 7, GU_EVENT_RISING, 100);

	status = gpiod_line_event_wait(line, &ts);
	GU_ASSERT_EQ(status, 1);

	status = gpiod_line_event_read(line, &ev);
	GU_ASSERT_RET_OK(status);

	GU_ASSERT_EQ(ev.event_type, GPIOD_EVENT_RISING_EDGE);
}
GU_DEFINE_TEST(event_rising_edge_active_low,
	       "events - single rising edge event with low active state",
	       GU_LINES_UNNAMED, { 8 });

static void event_get_value(void)
{
	GU_CLEANUP(gu_close_chip) struct gpiod_chip *chip = NULL;
	struct timespec ts = { 1, 0 };
	struct gpiod_line_event ev;
	struct gpiod_line *line;
	int status;

	chip = gpiod_chip_open(gu_chip_path(0));
	GU_ASSERT_NOT_NULL(chip);

	line = gpiod_chip_get_line(chip, 7);
	GU_ASSERT_NOT_NULL(line);

	status = gpiod_line_event_request_rising(line, "gpiod-unit", false);
	GU_ASSERT_RET_OK(status);

	status = gpiod_line_get_value(line);
	GU_ASSERT_EQ(status, 0);

	gu_set_event(0, 7, GU_EVENT_RISING, 100);

	status = gpiod_line_event_wait(line, &ts);
	GU_ASSERT_EQ(status, 1);

	status = gpiod_line_event_read(line, &ev);
	GU_ASSERT_RET_OK(status);

	GU_ASSERT_EQ(ev.event_type, GPIOD_EVENT_RISING_EDGE);

	status = gpiod_line_get_value(line);
	GU_ASSERT_EQ(status, 1);
}
GU_DEFINE_TEST(event_get_value,
	       "events - mixing events and gpiod_line_get_value()",
	       GU_LINES_UNNAMED, { 8 });
