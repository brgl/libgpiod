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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

int gpiod_simple_get_value(const char *device, unsigned int offset,
			   bool active_low, const char *consumer)
{
	int value, status;

	status = gpiod_simple_get_value_multiple(device, &offset, &value,
						 1, active_low, consumer);
	if (status < 0)
		return status;

	return value;
}

int gpiod_simple_get_value_multiple(const char *device,
				    const unsigned int *offsets, int *values,
				    unsigned int num_lines, bool active_low,
				    const char *consumer)
{
	struct gpiod_line_bulk bulk;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int status, flags;
	unsigned int i;

	if (num_lines > GPIOD_LINE_BULK_MAX_LINES) {
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

	flags = active_low ? GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW : 0;

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

int gpiod_simple_set_value(const char *device, unsigned int offset, int value,
			   bool active_low, const char *consumer,
			   gpiod_simple_set_value_cb cb, void *data)
{
	return gpiod_simple_set_value_multiple(device, &offset, &value, 1,
					       active_low, consumer, cb, data);
}

int gpiod_simple_set_value_multiple(const char *device,
				    const unsigned int *offsets,
				    const int *values, unsigned int num_lines,
				    bool active_low, const char *consumer,
				    gpiod_simple_set_value_cb cb, void *data)
{
	struct gpiod_line_bulk bulk;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int status, flags;
	unsigned int i;

	if (num_lines > GPIOD_LINE_BULK_MAX_LINES) {
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

	flags = active_low ? GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW : 0;

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

static int basic_event_poll(unsigned int num_lines,
			    struct gpiod_simple_event_poll_fd *fds,
			    const struct timespec *timeout,
			    void *data GPIOD_UNUSED)
{
	struct pollfd poll_fds[GPIOD_LINE_BULK_MAX_LINES];
	unsigned int i;
	int rv, ret;

	if (num_lines > GPIOD_LINE_BULK_MAX_LINES)
		return GPIOD_SIMPLE_EVENT_POLL_RET_ERR;

	memset(poll_fds, 0, sizeof(poll_fds));

	for (i = 0; i < num_lines; i++) {
		poll_fds[i].fd = fds[i].fd;
		poll_fds[i].events = POLLIN | POLLPRI;
	}

	rv = ppoll(poll_fds, num_lines, timeout, NULL);
	if (rv < 0) {
		if (errno == EINTR)
			return GPIOD_SIMPLE_EVENT_POLL_RET_TIMEOUT;
		else
			return GPIOD_SIMPLE_EVENT_POLL_RET_ERR;
	} else if (rv == 0) {
		return GPIOD_SIMPLE_EVENT_POLL_RET_TIMEOUT;
	}

	ret = rv;
	for (i = 0; i < num_lines; i++) {
		if (poll_fds[i].revents) {
			fds[i].event = true;
			if (!--rv)
				break;
		}
	}

	return ret;
}

int gpiod_simple_event_loop(const char *device, unsigned int offset,
			    bool active_low, const char *consumer,
			    const struct timespec *timeout,
			    gpiod_simple_event_poll_cb poll_cb,
			    gpiod_simple_event_handle_cb event_cb, void *data)
{
	return gpiod_simple_event_loop_multiple(device, &offset, 1, active_low,
						consumer, timeout, poll_cb,
						event_cb, data);
}

int gpiod_simple_event_loop_multiple(const char *device,
				     const unsigned int *offsets,
				     unsigned int num_lines, bool active_low,
				     const char *consumer,
				     const struct timespec *timeout,
				     gpiod_simple_event_poll_cb poll_cb,
				     gpiod_simple_event_handle_cb event_cb,
				     void *data)
{
	struct gpiod_simple_event_poll_fd fds[GPIOD_LINE_BULK_MAX_LINES];
	int rv, ret, flags, evtype, cnt;
	struct gpiod_line_event event;
	struct gpiod_line_bulk bulk;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	unsigned int i;

	if (num_lines > GPIOD_LINE_BULK_MAX_LINES) {
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

	flags = active_low ? GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW : 0;

	rv = gpiod_line_request_bulk_both_edges_events_flags(&bulk,
							      consumer, flags);
	if (rv) {
		ret = -1;
		goto out;
	}

	memset(fds, 0, sizeof(fds));
	for (i = 0; i < num_lines; i++) {
		line = gpiod_line_bulk_get_line(&bulk, i);
		fds[i].fd = gpiod_line_event_get_fd(line);
	}

	for (;;) {
		for (i = 0; i < num_lines; i++)
			fds[i].event = false;

		cnt = poll_cb(num_lines, fds, timeout, data);
		if (cnt == GPIOD_SIMPLE_EVENT_POLL_RET_ERR) {
			ret = -1;
			goto out;
		} else if (cnt == GPIOD_SIMPLE_EVENT_POLL_RET_TIMEOUT) {
			rv = event_cb(GPIOD_SIMPLE_EVENT_CB_TIMEOUT,
				      0, &event.ts, data);
			if (rv == GPIOD_SIMPLE_EVENT_CB_RET_ERR) {
				ret = -1;
				goto out;
			} else if (rv == GPIOD_SIMPLE_EVENT_CB_RET_STOP) {
				ret = 0;
				goto out;
			}
		} else if (cnt == GPIOD_SIMPLE_EVENT_POLL_RET_STOP) {
			ret = 0;
			goto out;
		}

		for (i = 0; i < num_lines; i++) {
			if (!fds[i].event)
				continue;

			line = gpiod_line_bulk_get_line(&bulk, i);
			rv = gpiod_line_event_read(line, &event);
			if (rv < 0) {
				ret = rv;
				goto out;
			}

			if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE)
				evtype = GPIOD_SIMPLE_EVENT_CB_RISING_EDGE;
			else
				evtype = GPIOD_SIMPLE_EVENT_CB_FALLING_EDGE;

			rv = event_cb(evtype, gpiod_line_offset(line),
				      &event.ts, data);
			if (rv == GPIOD_SIMPLE_EVENT_CB_RET_ERR) {
				ret = -1;
				goto out;
			} else if (rv == GPIOD_SIMPLE_EVENT_CB_RET_STOP) {
				ret = 0;
				goto out;
			}

			if (!--cnt)
				break;
		}
	}

out:
	gpiod_chip_close(chip);

	return ret;
}

int gpiod_simple_find_line(const char *name, char *chipname,
			   size_t chipname_size, unsigned int *offset)
{
	struct gpiod_chip *chip;
	struct gpiod_line *line;

	line = gpiod_line_find(name);
	if (!line) {
		if (errno == ENOENT)
			return 0;
		else
			return -1;
	}

	chip = gpiod_line_get_chip(line);
	snprintf(chipname, chipname_size, "%s", gpiod_chip_name(chip));
	*offset = gpiod_line_offset(line);
	gpiod_chip_close(chip);

	return 1;
}
