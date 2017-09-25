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
#include <poll.h>

#define UNUSED __attribute__((unused))

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
	int status, flags;
	unsigned int i;

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

	flags = active_low ? GPIOD_REQUEST_ACTIVE_LOW : 0;

	status = gpiod_line_request_bulk_input_flags(&bulk, consumer, flags);
	if (status < 0) {
		gpiod_chip_close(chip);
		return -1;
	}

	memset(values, 0, sizeof(*values) * num_lines);
	status = gpiod_line_get_value_bulk(&bulk, values);

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
	int status, flags;
	unsigned int i;

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

	flags = active_low ? GPIOD_REQUEST_ACTIVE_LOW : 0;

	status = gpiod_line_request_bulk_output_flags(&bulk, consumer,
						      flags, values);
	if (status < 0) {
		gpiod_chip_close(chip);
		return -1;
	}

	if (cb)
		cb(data);

	gpiod_chip_close(chip);

	return 0;
}

static int basic_event_poll(unsigned int num_lines, const int *fds,
			    unsigned int *event_offset,
			    const struct timespec *timeout,
			    void *data UNUSED)
{
	struct pollfd poll_fds[GPIOD_REQUEST_MAX_LINES];
	unsigned int i;
	int status;

	memset(poll_fds, 0, sizeof(poll_fds));

	for (i = 0; i < num_lines; i++) {
		poll_fds[i].fd = fds[i];
		poll_fds[i].events = POLLIN | POLLPRI;
	}

	status = ppoll(poll_fds, num_lines, timeout, NULL);
	if (status < 0) {
		if (errno == EINTR)
			return GPIOD_EVENT_POLL_TIMEOUT;
		else
			return GPIOD_EVENT_POLL_ERR;
	} else if (status == 0) {
		return GPIOD_EVENT_POLL_TIMEOUT;
	}

	for (i = 0; !poll_fds[i].revents; i++);
	*event_offset = i;

	return GPIOD_EVENT_POLL_EVENT;
}

int gpiod_simple_event_loop(const char *consumer, const char *device,
			    unsigned int offset, bool active_low,
			    const struct timespec *timeout,
			    gpiod_event_poll_cb poll_cb,
			    gpiod_event_handle_cb event_cb,
			    void *data)
{
	return gpiod_simple_event_loop_multiple(consumer, device, &offset, 1,
						active_low, timeout, poll_cb,
						event_cb, data);
}

int gpiod_simple_event_loop_multiple(const char *consumer, const char *device,
				     const unsigned int *offsets,
				     unsigned int num_lines, bool active_low,
				     const struct timespec *timeout,
				     gpiod_event_poll_cb poll_cb,
				     gpiod_event_handle_cb event_cb,
				     void *data)
{
	unsigned int i, event_offset, line_offset;
	int fds[GPIOD_REQUEST_MAX_LINES];
	struct gpiod_line_event event;
	struct gpiod_line_bulk bulk;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int ret, flags, evtype;

	if (num_lines > GPIOD_REQUEST_MAX_LINES) {
		errno = EINVAL;
		return -1;
	}

	if (!poll_cb)
		poll_cb = basic_event_poll;

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

	flags = active_low ? GPIOD_REQUEST_ACTIVE_LOW : 0;

	ret = gpiod_line_request_bulk_both_edges_events_flags(&bulk,
							      consumer, flags);
	if (ret) {
		gpiod_chip_close(chip);
		return ret;
	}

	memset(fds, 0, sizeof(fds));
	for (i = 0; i < num_lines; i++)
		fds[i] = gpiod_line_event_get_fd(
				gpiod_line_bulk_get_line(&bulk, i));

	for (;;) {
		ret = poll_cb(num_lines, fds, &event_offset, timeout, data);
		if (ret < 0) {
			goto out;
		} else if (ret == GPIOD_EVENT_POLL_TIMEOUT) {
			evtype = GPIOD_EVENT_CB_TIMEOUT;
			line_offset = 0;
		} else if (ret == GPIOD_EVENT_POLL_STOP) {
			ret = 0;
			goto out;
		} else {
			line = gpiod_line_bulk_get_line(&bulk, event_offset);
			ret = gpiod_line_event_read(line, &event);
			if (ret < 0)
				goto out;

			evtype = event.event_type == GPIOD_EVENT_RISING_EDGE
						? GPIOD_EVENT_CB_RISING_EDGE
						: GPIOD_EVENT_CB_FALLING_EDGE;

			line_offset = offsets[event_offset];
		}

		ret = event_cb(evtype, line_offset, &event.ts, data);
		if (ret == GPIOD_EVENT_CB_STOP) {
			ret = 0;
			goto out;
		}
	}

out:
	gpiod_chip_close(chip);

	return ret;
}
