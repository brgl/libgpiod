/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

/* Low-level, core library code. */

#include <gpiod.h>

#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

enum {
	LINE_FREE = 0,
	LINE_REQUESTED_VALUES,
	LINE_REQUESTED_EVENTS,
};

struct gpiod_line {
	int state;
	bool up_to_date;
	struct gpiod_chip *chip;
	struct gpioline_info info;
	int fd;
};

struct gpiod_chip {
	int fd;
	struct gpiochip_info cinfo;
	struct gpiod_line **lines;
};

struct gpiod_chip * gpiod_chip_open(const char *path)
{
	struct gpiod_chip *chip;
	int status, fd;

	fd = open(path, O_RDWR | O_CLOEXEC);
	if (fd < 0)
		return NULL;

	chip = malloc(sizeof(*chip));
	if (!chip) {
		close(fd);
		return NULL;
	}

	memset(chip, 0, sizeof(*chip));
	chip->fd = fd;

	status = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &chip->cinfo);
	if (status < 0) {
		close(chip->fd);
		free(chip);
		return NULL;
	}

	return chip;
}

void gpiod_chip_close(struct gpiod_chip *chip)
{
	struct gpiod_line *line;
	unsigned int i;

	if (chip->lines) {
		for (i = 0; i < chip->cinfo.lines; i++) {
			line = chip->lines[i];
			if (line) {
				gpiod_line_release(line);
				free(line);
			}
		}

		free(chip->lines);
	}

	close(chip->fd);
	free(chip);
}

const char * gpiod_chip_name(struct gpiod_chip *chip)
{
	return chip->cinfo.name[0] == '\0' ? NULL : chip->cinfo.name;
}

const char * gpiod_chip_label(struct gpiod_chip *chip)
{
	return chip->cinfo.label[0] == '\0' ? NULL : chip->cinfo.label;
}

unsigned int gpiod_chip_num_lines(struct gpiod_chip *chip)
{
	return (unsigned int)chip->cinfo.lines;
}

struct gpiod_line *
gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset)
{
	struct gpiod_line *line;
	int status;

	if (offset >= chip->cinfo.lines) {
		errno = EINVAL;
		return NULL;
	}

	if (!chip->lines) {
		chip->lines = calloc(chip->cinfo.lines,
				     sizeof(struct gpiod_line *));
		if (!chip->lines)
			return NULL;
	}

	if (!chip->lines[offset]) {
		line = malloc(sizeof(*line));
		if (!line)
			return NULL;

		memset(line, 0, sizeof(*line));
		line->fd = -1;

		line->info.line_offset = offset;
		line->chip = chip;

		chip->lines[offset] = line;
	} else {
		line = chip->lines[offset];
	}

	status = gpiod_line_update(line);
	if (status < 0)
		return NULL;

	return line;
}

static void line_maybe_update(struct gpiod_line *line)
{
	int status;

	status = gpiod_line_update(line);
	if (status < 0)
		line->up_to_date = false;
}

struct gpiod_chip * gpiod_line_get_chip(struct gpiod_line *line)
{
	return line->chip;
}

unsigned int gpiod_line_offset(struct gpiod_line *line)
{
	return (unsigned int)line->info.line_offset;
}

const char * gpiod_line_name(struct gpiod_line *line)
{
	return line->info.name[0] == '\0' ? NULL : line->info.name;
}

const char * gpiod_line_consumer(struct gpiod_line *line)
{
	return line->info.consumer[0] == '\0' ? NULL : line->info.consumer;
}

int gpiod_line_direction(struct gpiod_line *line)
{
	return line->info.flags & GPIOLINE_FLAG_IS_OUT
						? GPIOD_LINE_DIRECTION_OUTPUT
						: GPIOD_LINE_DIRECTION_INPUT;
}

int gpiod_line_active_state(struct gpiod_line *line)
{
	return line->info.flags & GPIOLINE_FLAG_ACTIVE_LOW
					? GPIOD_LINE_ACTIVE_STATE_LOW
					: GPIOD_LINE_ACTIVE_STATE_HIGH;
}

bool gpiod_line_is_used(struct gpiod_line *line)
{
	return line->info.flags & GPIOLINE_FLAG_KERNEL;
}

bool gpiod_line_is_open_drain(struct gpiod_line *line)
{
	return line->info.flags & GPIOLINE_FLAG_OPEN_DRAIN;
}

bool gpiod_line_is_open_source(struct gpiod_line *line)
{
	return line->info.flags & GPIOLINE_FLAG_OPEN_SOURCE;
}

bool gpiod_line_needs_update(struct gpiod_line *line)
{
	return !line->up_to_date;
}

int gpiod_line_update(struct gpiod_line *line)
{
	int rv;

	memset(line->info.name, 0, sizeof(line->info.name));
	memset(line->info.consumer, 0, sizeof(line->info.consumer));
	line->info.flags = 0;

	rv = ioctl(line->chip->fd, GPIO_GET_LINEINFO_IOCTL, &line->info);
	if (rv < 0)
		return -1;

	line->up_to_date = true;

	return 0;
}

static bool line_bulk_is_requested(struct gpiod_line_bulk *bulk)
{
	struct gpiod_line *line, **lineptr;

	gpiod_line_bulk_foreach_line(bulk, line, lineptr) {
		if (!gpiod_line_is_requested(line))
			return false;
	}

	return true;
}

static bool verify_line_bulk(struct gpiod_line_bulk *bulk)
{
	struct gpiod_line *line, **lineptr;
	struct gpiod_chip *chip;
	int i;

	chip = gpiod_line_get_chip(gpiod_line_bulk_get_line(bulk, 0));

	i = 0;
	gpiod_line_bulk_foreach_line(bulk, line, lineptr) {
		if (i++ > 0 && chip != gpiod_line_get_chip(line)) {
			errno = EINVAL;
			return false;
		}

		if (!gpiod_line_is_free(line)) {
			errno = EBUSY;
			return false;
		}
	}

	return true;
}

static int line_request_values(struct gpiod_line_bulk *bulk,
			       const struct gpiod_line_request_config *config,
			       const int *default_vals)
{
	struct gpiod_line *line, **lineptr;
	struct gpiohandle_request req;
	unsigned int i;
	int rv, fd;

	if ((config->request_type != GPIOD_LINE_REQUEST_DIRECTION_OUTPUT) &&
	    (config->flags & (GPIOD_LINE_REQUEST_OPEN_DRAIN |
			      GPIOD_LINE_REQUEST_OPEN_SOURCE))) {
		errno = EINVAL;
		return -1;
	}

	memset(&req, 0, sizeof(req));

	if (config->flags & GPIOD_LINE_REQUEST_OPEN_DRAIN)
		req.flags |= GPIOHANDLE_REQUEST_OPEN_DRAIN;
	if (config->flags & GPIOD_LINE_REQUEST_OPEN_SOURCE)
		req.flags |= GPIOHANDLE_REQUEST_OPEN_SOURCE;
	if (config->flags & GPIOD_LINE_REQUEST_ACTIVE_LOW)
		req.flags |= GPIOHANDLE_REQUEST_ACTIVE_LOW;

	if (config->request_type == GPIOD_LINE_REQUEST_DIRECTION_INPUT)
		req.flags |= GPIOHANDLE_REQUEST_INPUT;
	else if (config->request_type == GPIOD_LINE_REQUEST_DIRECTION_OUTPUT)
		req.flags |= GPIOHANDLE_REQUEST_OUTPUT;

	req.lines = gpiod_line_bulk_num_lines(bulk);

	for (i = 0; i < gpiod_line_bulk_num_lines(bulk); i++) {
		line = gpiod_line_bulk_get_line(bulk, i);
		req.lineoffsets[i] = gpiod_line_offset(line);
		if (config->request_type == GPIOD_LINE_REQUEST_DIRECTION_OUTPUT)
			req.default_values[i] = !!default_vals[i];
	}

	if (config->consumer)
		strncpy(req.consumer_label, config->consumer,
			sizeof(req.consumer_label) - 1);

	line = gpiod_line_bulk_get_line(bulk, 0);
	fd = line->chip->fd;

	rv = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, &req);
	if (rv < 0)
		return -1;

	gpiod_line_bulk_foreach_line(bulk, line, lineptr) {
		line->state = LINE_REQUESTED_VALUES;
		line->fd = req.fd;
		line_maybe_update(line);
	}

	return 0;
}

static int line_request_event_single(struct gpiod_line *line,
			const struct gpiod_line_request_config *config)
{
	struct gpioevent_request req;
	int rv;

	memset(&req, 0, sizeof(req));

	if (config->consumer)
		strncpy(req.consumer_label, config->consumer,
			sizeof(req.consumer_label) - 1);

	req.lineoffset = gpiod_line_offset(line);
	req.handleflags |= GPIOHANDLE_REQUEST_INPUT;

	if (config->flags & GPIOD_LINE_REQUEST_OPEN_DRAIN)
		req.handleflags |= GPIOHANDLE_REQUEST_OPEN_DRAIN;
	if (config->flags & GPIOD_LINE_REQUEST_OPEN_SOURCE)
		req.handleflags |= GPIOHANDLE_REQUEST_OPEN_SOURCE;
	if (config->flags & GPIOD_LINE_REQUEST_ACTIVE_LOW)
		req.handleflags |= GPIOHANDLE_REQUEST_ACTIVE_LOW;

	if (config->request_type == GPIOD_LINE_REQUEST_EVENT_RISING_EDGE)
		req.eventflags |= GPIOEVENT_REQUEST_RISING_EDGE;
	else if (config->request_type == GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE)
		req.eventflags |= GPIOEVENT_REQUEST_FALLING_EDGE;
	else if (config->request_type == GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES)
		req.eventflags |= GPIOEVENT_REQUEST_BOTH_EDGES;

	rv = ioctl(line->chip->fd, GPIO_GET_LINEEVENT_IOCTL, &req);
	if (rv < 0)
		return -1;

	line->state = LINE_REQUESTED_EVENTS;
	line->fd = req.fd;

	return 0;
}

static int line_request_events(struct gpiod_line_bulk *bulk,
			       const struct gpiod_line_request_config *config)
{
	struct gpiod_line *line, **lineptr;
	unsigned int i, j;
	int rv;

	i = 0;
	gpiod_line_bulk_foreach_line(bulk, line, lineptr) {
		rv = line_request_event_single(line, config);
		if (rv) {
			for (j = i - 1; i > 0; i--) {
				line = gpiod_line_bulk_get_line(bulk, j);
				gpiod_line_release(line);
			}

			return -1;
		}

		i++;
	}

	return 0;
}

int gpiod_line_request(struct gpiod_line *line,
		       const struct gpiod_line_request_config *config,
		       int default_val)
{
	struct gpiod_line_bulk bulk;

	gpiod_line_bulk_init(&bulk);
	gpiod_line_bulk_add(&bulk, line);

	return gpiod_line_request_bulk(&bulk, config, &default_val);
}

static bool line_request_is_direction(int request)
{
	return request == GPIOD_LINE_REQUEST_DIRECTION_AS_IS ||
	       request == GPIOD_LINE_REQUEST_DIRECTION_INPUT ||
	       request == GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
}

static bool line_request_is_events(int request)
{
	return request == GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE ||
	       request == GPIOD_LINE_REQUEST_EVENT_RISING_EDGE ||
	       request == GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
}

int gpiod_line_request_bulk(struct gpiod_line_bulk *bulk,
			    const struct gpiod_line_request_config *config,
			    const int *default_vals)
{
	if (!verify_line_bulk(bulk))
		return -1;

	if (line_request_is_direction(config->request_type)) {
		return line_request_values(bulk, config, default_vals);
	} else if (line_request_is_events(config->request_type)) {
		return line_request_events(bulk, config);
	} else {
		errno = EINVAL;
		return -1;
	}
}

void gpiod_line_release(struct gpiod_line *line)
{
	struct gpiod_line_bulk bulk;

	gpiod_line_bulk_init(&bulk);
	gpiod_line_bulk_add(&bulk, line);

	gpiod_line_release_bulk(&bulk);
}

void gpiod_line_release_bulk(struct gpiod_line_bulk *bulk)
{
	struct gpiod_line *line, **lineptr;

	gpiod_line_bulk_foreach_line(bulk, line, lineptr) {
		if (line->state != LINE_FREE) {
			close(line->fd);
			line->state = LINE_FREE;
		}
	}
}

bool gpiod_line_is_requested(struct gpiod_line *line)
{
	return (line->state == LINE_REQUESTED_VALUES ||
		line->state == LINE_REQUESTED_EVENTS);
}

bool gpiod_line_is_free(struct gpiod_line *line)
{
	return line->state == LINE_FREE;
}

int gpiod_line_get_value(struct gpiod_line *line)
{
	struct gpiod_line_bulk bulk;
	int status, value;

	gpiod_line_bulk_init(&bulk);
	gpiod_line_bulk_add(&bulk, line);

	status = gpiod_line_get_value_bulk(&bulk, &value);
	if (status < 0)
		return -1;

	return value;
}

int gpiod_line_get_value_bulk(struct gpiod_line_bulk *bulk, int *values)
{
	struct gpiohandle_data data;
	struct gpiod_line *first;
	unsigned int i;
	int status, fd;

	first = gpiod_line_bulk_get_line(bulk, 0);

	if (!line_bulk_is_requested(bulk)) {
		errno = EPERM;
		return -1;
	}

	memset(&data, 0, sizeof(data));

	fd = first->fd;

	status = ioctl(fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
	if (status < 0)
		return -1;

	for (i = 0; i < gpiod_line_bulk_num_lines(bulk); i++)
		values[i] = data.values[i];

	return 0;
}

int gpiod_line_set_value(struct gpiod_line *line, int value)
{
	struct gpiod_line_bulk bulk;

	gpiod_line_bulk_init(&bulk);
	gpiod_line_bulk_add(&bulk, line);

	return gpiod_line_set_value_bulk(&bulk, &value);
}

int gpiod_line_set_value_bulk(struct gpiod_line_bulk *bulk, int *values)
{
	struct gpiohandle_data data;
	struct gpiod_line *line;
	unsigned int i;
	int status;

	if (!line_bulk_is_requested(bulk)) {
		errno = EPERM;
		return -1;
	}

	memset(&data, 0, sizeof(data));

	for (i = 0; i < gpiod_line_bulk_num_lines(bulk); i++)
		data.values[i] = (uint8_t)!!values[i];

	line = gpiod_line_bulk_get_line(bulk, 0);
	status = ioctl(line->fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
	if (status < 0)
		return -1;

	return 0;
}

int gpiod_line_event_wait(struct gpiod_line *line,
			  const struct timespec *timeout)
{
	struct gpiod_line_bulk bulk;

	gpiod_line_bulk_init(&bulk);
	gpiod_line_bulk_add(&bulk, line);

	return gpiod_line_event_wait_bulk(&bulk, timeout, NULL);
}

int gpiod_line_event_wait_bulk(struct gpiod_line_bulk *bulk,
			       const struct timespec *timeout,
			       struct gpiod_line_bulk *event_bulk)
{
	struct pollfd fds[GPIOD_LINE_BULK_MAX_LINES];
	struct gpiod_line *line, **lineptr;
	unsigned int i, num_lines;
	int rv;

	memset(fds, 0, sizeof(fds));
	num_lines = gpiod_line_bulk_num_lines(bulk);

	i = 0;
	gpiod_line_bulk_foreach_line(bulk, line, lineptr) {
		fds[i].fd = line->fd;
		fds[i].events = POLLIN | POLLPRI;
		i++;
	}

	rv = ppoll(fds, num_lines, timeout, NULL);
	if (rv < 0)
		return -1;
	else if (rv == 0)
		return 0;

	if (event_bulk) {
		gpiod_line_bulk_init(event_bulk);

		for (i = 0; i < num_lines; i++) {
			if (fds[i].revents) {
				line = gpiod_line_bulk_get_line(bulk, i);
				gpiod_line_bulk_add(event_bulk, line);
				if (!--rv)
					break;
			}
		}
	}

	return 1;
}

int gpiod_line_event_read(struct gpiod_line *line,
			  struct gpiod_line_event *event)
{
	return gpiod_line_event_read_fd(line->fd, event);
}

int gpiod_line_event_get_fd(struct gpiod_line *line)
{
	if (line->state != LINE_REQUESTED_EVENTS) {
		errno = EINVAL;
		return -1;
	}

	return line->fd;
}

int gpiod_line_event_read_fd(int fd, struct gpiod_line_event *event)
{
	struct gpioevent_data evdata;
	ssize_t rd;

	memset(&evdata, 0, sizeof(evdata));

	rd = read(fd, &evdata, sizeof(evdata));
	if (rd < 0) {
		return -1;
	} else if (rd != sizeof(evdata)) {
		errno = EIO;
		return -1;
	}

	event->event_type = evdata.id == GPIOEVENT_EVENT_RISING_EDGE
						? GPIOD_LINE_EVENT_RISING_EDGE
						: GPIOD_LINE_EVENT_FALLING_EDGE;

	event->ts.tv_sec = evdata.timestamp / 1000000000ULL;
	event->ts.tv_nsec = evdata.timestamp % 1000000000ULL;

	return 0;
}
