/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

/* Implementation of the high-level API. */

#include <gpiod.h>

#include <string.h>
#include <errno.h>

int gpiod_simple_get_value(const char *consumer, const char *device,
			   unsigned int offset, bool active_low)
{
	int value, status;

	status = gpiod_simple_get_value_multiple(consumer, device, &offset,
						 &value, 1, active_low);
	if (status < 0)
		return status;

	return value;
}

int gpiod_simple_get_value_multiple(const char *consumer, const char *device,
				    const unsigned int *offsets, int *values,
				    unsigned int num_lines, bool active_low)
{
	struct gpiod_line_bulk bulk;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	unsigned int i;
	int status;

	if (num_lines > GPIOD_REQUEST_MAX_LINES) {
		errno = EINVAL;
		return -1;
	}

	chip = gpiod_chip_open_lookup(device);
	if (!chip)
		return -1;

	gpiod_line_bulk_init(&bulk);

	for (i = 0; i < num_lines; i++) {
		line = gpiod_chip_get_line(chip, offsets[i]);
		if (!line) {
			gpiod_chip_close(chip);
			return -1;
		}

		gpiod_line_bulk_add(&bulk, line);
	}

	status = gpiod_line_request_bulk_input(&bulk, consumer, active_low);
	if (status < 0) {
		gpiod_chip_close(chip);
		return -1;
	}

	memset(values, 0, sizeof(*values) * num_lines);
	status = gpiod_line_get_value_bulk(&bulk, values);

	gpiod_line_release_bulk(&bulk);
	gpiod_chip_close(chip);

	return status;
}

int gpiod_simple_set_value(const char *consumer, const char *device,
			   unsigned int offset, int value, bool active_low,
			   gpiod_set_value_cb cb, void *data)
{
	return gpiod_simple_set_value_multiple(consumer, device, &offset,
					       &value, 1, active_low,
					       cb, data);
}

int gpiod_simple_set_value_multiple(const char *consumer, const char *device,
				    const unsigned int *offsets,
				    const int *values, unsigned int num_lines,
				    bool active_low, gpiod_set_value_cb cb,
				    void *data)
{
	struct gpiod_line_bulk bulk;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	unsigned int i;
	int status;

	if (num_lines > GPIOD_REQUEST_MAX_LINES) {
		errno = EINVAL;
		return -1;
	}

	chip = gpiod_chip_open_lookup(device);
	if (!chip)
		return -1;

	gpiod_line_bulk_init(&bulk);

	for (i = 0; i < num_lines; i++) {
		line = gpiod_chip_get_line(chip, offsets[i]);
		if (!line) {
			gpiod_chip_close(chip);
			return -1;
		}

		gpiod_line_bulk_add(&bulk, line);
	}

	status = gpiod_line_request_bulk_output(&bulk, consumer,
						active_low, values);
	if (status < 0) {
		gpiod_chip_close(chip);
		return -1;
	}

	if (cb)
		cb(data);

	gpiod_line_release_bulk(&bulk);
	gpiod_chip_close(chip);

	return 0;
}

int gpiod_simple_event_loop(const char *consumer, const char *device,
			    unsigned int offset, bool active_low,
			    const struct timespec *timeout,
			    gpiod_event_cb callback, void *cbdata)
{
	struct gpiod_line_event event;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int status, evtype;

	chip = gpiod_chip_open_lookup(device);
	if (!chip)
		return -1;

	line = gpiod_chip_get_line(chip, offset);
	if (!line) {
		gpiod_chip_close(chip);
		return -1;
	}

	status = gpiod_line_request_both_edges_events_flags(line, consumer,
							    active_low);
	if (status < 0) {
		gpiod_chip_close(chip);
		return -1;
	}

	for (;;) {
		status = gpiod_line_event_wait(line, timeout);
		if (status < 0) {
			if (errno == EINTR)
				return evtype = GPIOD_EVENT_CB_TIMEOUT;
			else
				goto out;
		} else if (status == 0) {
			evtype = GPIOD_EVENT_CB_TIMEOUT;
		} else {
			status = gpiod_line_event_read(line, &event);
			if (status < 0)
				goto out;

			evtype = event.event_type == GPIOD_EVENT_RISING_EDGE
						? GPIOD_EVENT_CB_RISING_EDGE
						: GPIOD_EVENT_CB_FALLING_EDGE;
		}

		status = callback(evtype, &event.ts, cbdata);
		if (status == GPIOD_EVENT_CB_STOP) {
			status = 0;
			goto out;
		}
	}

out:
	gpiod_line_release(line);
	gpiod_chip_close(chip);

	return status;
}
