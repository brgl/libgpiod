/*
 * GPIO line API for libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "gpiod-internal.h"

#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

enum {
	LINE_FREE = 0,
	LINE_TAKEN,
	LINE_EVENT,
};

struct handle_data {
	struct gpiohandle_request request;
	int refcount;
};

struct gpiod_line {
	int state;
	bool up_to_date;
	struct gpiod_chip *chip;
	struct gpioline_info info;
	union {
		struct handle_data *handle;
		struct gpioevent_request event;
	};
};

static int line_get_state(struct gpiod_line *line)
{
	return line->state;
}

static void line_set_state(struct gpiod_line *line, int state)
{
	line->state = state;
}

static int line_get_handle_fd(struct gpiod_line *line)
{
	int state = line_get_state(line);

	return state == LINE_TAKEN ? line->handle->request.fd : -1;
}

static int line_get_event_fd(struct gpiod_line *line)
{
	return line_get_state(line) == LINE_EVENT ? line->event.fd : -1;
}

static void line_set_handle(struct gpiod_line *line,
			    struct handle_data *handle)
{
	line->handle = handle;
	handle->refcount++;
}

static void line_remove_handle(struct gpiod_line *line)
{
	struct handle_data *handle;

	if (!line->handle)
		return;

	handle = line->handle;
	line->handle = NULL;
	handle->refcount--;
	if (handle->refcount <= 0) {
		close(handle->request.fd);
		free(handle);
	}
}

void line_set_offset(struct gpiod_line *line, unsigned int offset)
{
	line->info.line_offset = offset;
}

struct gpiod_chip * gpiod_line_get_chip(struct gpiod_line *line)
{
	return line->chip;
}

struct gpiod_line * line_array_alloc(size_t numlines)
{
	return calloc(numlines, sizeof(struct gpiod_line));
}

void line_array_free(struct gpiod_line *lines)
{
	free(lines);
}

struct gpiod_line * line_array_member(struct gpiod_line *lines, size_t index)
{
	return &lines[index];
}

void line_set_chip(struct gpiod_line *line, struct gpiod_chip *chip)
{
	line->chip = chip;
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
	return line->info.flags & GPIOLINE_FLAG_IS_OUT ? GPIOD_DIRECTION_OUTPUT
						       : GPIOD_DIRECTION_INPUT;
}

int gpiod_line_active_state(struct gpiod_line *line)
{
	return line->info.flags & GPIOLINE_FLAG_ACTIVE_LOW
					? GPIOD_ACTIVE_STATE_LOW
					: GPIOD_ACTIVE_STATE_HIGH;
}

bool gpiod_line_is_used_by_kernel(struct gpiod_line *line)
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

static void line_set_updated(struct gpiod_line *line)
{
	line->up_to_date = true;
}

static void line_set_needs_update(struct gpiod_line *line)
{
	line->up_to_date = false;
}

static void line_maybe_update(struct gpiod_line *line)
{
	int status;

	status = gpiod_line_update(line);
	if (status < 0)
		line_set_needs_update(line);
}

static bool line_bulk_is_reserved(struct gpiod_line_bulk *bulk)
{
	unsigned int i;

	for (i = 0; i < bulk->num_lines; i++) {
		if (!gpiod_line_is_reserved(bulk->lines[i]))
			return false;
	}

	return true;
}

static bool line_bulk_is_event_configured(struct gpiod_line_bulk *bulk)
{
	unsigned int i;

	for (i = 0; i < bulk->num_lines; i++) {
		if (!gpiod_line_event_configured(bulk->lines[i]))
			return false;
	}

	return true;
}

bool gpiod_line_needs_update(struct gpiod_line *line)
{
	return !line->up_to_date;
}

int gpiod_line_update(struct gpiod_line *line)
{
	struct gpiod_chip *chip;
	int status, fd;

	memset(line->info.name, 0, sizeof(line->info.name));
	memset(line->info.consumer, 0, sizeof(line->info.consumer));
	line->info.flags = 0;

	chip = gpiod_line_get_chip(line);
	fd = chip_get_fd(chip);;

	status = ioctl(fd, GPIO_GET_LINEINFO_IOCTL, &line->info);
	if (status < 0)
		return -1;

	line_set_updated(line);

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

int gpiod_line_request_input(struct gpiod_line *line,
			     const char *consumer, bool active_low)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.direction = GPIOD_DIRECTION_INPUT,
		.active_state = active_low ? GPIOD_ACTIVE_STATE_LOW
					   : GPIOD_ACTIVE_STATE_HIGH,
	};

	return gpiod_line_request(line, &config, 0);
}

int gpiod_line_request_output(struct gpiod_line *line, const char *consumer,
			      bool active_low, int default_val)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.direction = GPIOD_DIRECTION_OUTPUT,
		.active_state = active_low ? GPIOD_ACTIVE_STATE_LOW
					   : GPIOD_ACTIVE_STATE_HIGH,
	};

	return gpiod_line_request(line, &config, default_val);
}

static bool verify_line_bulk(struct gpiod_line_bulk *bulk)
{
	struct gpiod_line *line;
	struct gpiod_chip *chip;
	unsigned int i;

	chip = gpiod_line_get_chip(bulk->lines[0]);

	for (i = 0; i < bulk->num_lines; i++) {
		line = bulk->lines[i];

		if (i > 0 && chip != gpiod_line_get_chip(line)) {
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

int gpiod_line_request_bulk(struct gpiod_line_bulk *bulk,
			    const struct gpiod_line_request_config *config,
			    const int *default_vals)
{
	struct gpiohandle_request *req;
	struct handle_data *handle;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int status, fd;
	unsigned int i;

	if (!verify_line_bulk(bulk))
		return -1;

	handle = malloc(sizeof(*handle));
	if (!handle)
		return -1;

	memset(handle, 0, sizeof(*handle));

	req = &handle->request;

	if (config->flags & GPIOD_REQUEST_OPEN_DRAIN)
		req->flags |= GPIOHANDLE_REQUEST_OPEN_DRAIN;
	if (config->flags & GPIOD_REQUEST_OPEN_SOURCE)
		req->flags |= GPIOHANDLE_REQUEST_OPEN_SOURCE;

	if (config->direction == GPIOD_DIRECTION_INPUT)
		req->flags |= GPIOHANDLE_REQUEST_INPUT;
	else if (config->direction == GPIOD_DIRECTION_OUTPUT)
		req->flags |= GPIOHANDLE_REQUEST_OUTPUT;

	if (config->active_state == GPIOD_ACTIVE_STATE_LOW)
		req->flags |= GPIOHANDLE_REQUEST_ACTIVE_LOW;

	req->lines = bulk->num_lines;

	for (i = 0; i < bulk->num_lines; i++) {
		req->lineoffsets[i] = gpiod_line_offset(bulk->lines[i]);
		if (config->direction == GPIOD_DIRECTION_OUTPUT)
			req->default_values[i] = !!default_vals[i];
	}

	strncpy(req->consumer_label, config->consumer,
		sizeof(req->consumer_label) - 1);

	chip = gpiod_line_get_chip(bulk->lines[0]);
	fd = chip_get_fd(chip);;

	status = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, req);
	if (status < 0)
		return -1;

	for (i = 0; i < bulk->num_lines; i++) {
		line = bulk->lines[i];

		line_set_handle(line, handle);
		line_set_state(line, LINE_TAKEN);
		line_maybe_update(line);
	}

	return 0;
}

int gpiod_line_request_bulk_input(struct gpiod_line_bulk *bulk,
				  const char *consumer, bool active_low)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.direction = GPIOD_DIRECTION_INPUT,
		.active_state = active_low ? GPIOD_ACTIVE_STATE_LOW
					   : GPIOD_ACTIVE_STATE_HIGH,
	};

	return gpiod_line_request_bulk(bulk, &config, 0);
}

int gpiod_line_request_bulk_output(struct gpiod_line_bulk *bulk,
				   const char *consumer, bool active_low,
				   const int *default_vals)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.direction = GPIOD_DIRECTION_OUTPUT,
		.active_state = active_low ? GPIOD_ACTIVE_STATE_LOW
					   : GPIOD_ACTIVE_STATE_HIGH,
	};

	return gpiod_line_request_bulk(bulk, &config, default_vals);
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
	struct gpiod_line *line;
	unsigned int i;

	for (i = 0; i < bulk->num_lines; i++) {
		line = bulk->lines[i];

		if (line_get_state(line) == LINE_TAKEN)
			line_remove_handle(line);
		else if (line_get_state(line) == LINE_EVENT)
			close(line_get_event_fd(line));

		line_set_state(line, LINE_FREE);
		/*
		 * FIXME This is a temporary check before we add a better way
		 * to determine whether a line object is valid.
		 */
		if (line->chip)
			line_maybe_update(line);
	}
}

bool gpiod_line_is_reserved(struct gpiod_line *line)
{
	return line_get_state(line) == LINE_TAKEN;
}

bool gpiod_line_is_free(struct gpiod_line *line)
{
	return line_get_state(line) == LINE_FREE;
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

	first = bulk->lines[0];

	if (!line_bulk_is_reserved(bulk) &&
	    !line_bulk_is_event_configured(bulk)) {
		errno = EPERM;
		return -1;
	}

	memset(&data, 0, sizeof(data));

	if (gpiod_line_is_reserved(first))
		fd = line_get_handle_fd(first);
	else
		fd = line_get_event_fd(first);

	status = ioctl(fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
	if (status < 0)
		return -1;

	for (i = 0; i < bulk->num_lines; i++)
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
	unsigned int i;
	int status;

	if (!line_bulk_is_reserved(bulk)) {
		errno = EPERM;
		return -1;
	}

	memset(&data, 0, sizeof(data));

	for (i = 0; i < bulk->num_lines; i++)
		data.values[i] = (uint8_t)!!values[i];

	status = ioctl(line_get_handle_fd(bulk->lines[0]),
			    GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
	if (status < 0)
		return -1;

	return 0;
}

struct gpiod_line * gpiod_line_find_by_name(const char *name)
{
	struct gpiod_chip_iter *chip_iter;
	struct gpiod_line_iter line_iter;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	const char *line_name;

	chip_iter = gpiod_chip_iter_new();
	if (!chip_iter)
		return NULL;

	gpiod_foreach_chip(chip_iter, chip) {
		if (gpiod_chip_iter_err(chip_iter))
			continue;

		gpiod_line_iter_init(&line_iter, chip);
		gpiod_foreach_line(&line_iter, line) {
			if (gpiod_line_iter_err(&line_iter))
				continue;

			line_name = gpiod_line_name(line);
			if (!line_name)
				continue;

			if (strcmp(line_name, name) == 0) {
				gpiod_chip_iter_free_noclose(chip_iter);
				return line;
			}
		}
	}

	gpiod_chip_iter_free(chip_iter);

	return NULL;
}

int gpiod_line_event_request(struct gpiod_line *line,
			     struct gpiod_line_evreq_config *config)
{
	struct gpioevent_request *req;
	struct gpiod_chip *chip;
	int status, fd;

	if (!gpiod_line_is_free(line)) {
		errno = EBUSY;
		return -1;
	}

	req = &line->event;

	memset(req, 0, sizeof(*req));
	strncpy(req->consumer_label, config->consumer,
		sizeof(req->consumer_label) - 1);
	req->lineoffset = gpiod_line_offset(line);
	req->handleflags |= GPIOHANDLE_REQUEST_INPUT;

	if (config->line_flags & GPIOD_REQUEST_OPEN_DRAIN)
		req->handleflags |= GPIOHANDLE_REQUEST_OPEN_DRAIN;
	if (config->line_flags & GPIOD_REQUEST_OPEN_SOURCE)
		req->handleflags |= GPIOHANDLE_REQUEST_OPEN_SOURCE;

	if (config->active_state == GPIOD_ACTIVE_STATE_LOW)
		req->handleflags |= GPIOHANDLE_REQUEST_ACTIVE_LOW;

	if (config->event_type == GPIOD_EVENT_RISING_EDGE)
		req->eventflags |= GPIOEVENT_REQUEST_RISING_EDGE;
	else if (config->event_type == GPIOD_EVENT_FALLING_EDGE)
		req->eventflags |= GPIOEVENT_REQUEST_FALLING_EDGE;
	else if (config->event_type == GPIOD_EVENT_BOTH_EDGES)
		req->eventflags |= GPIOEVENT_REQUEST_BOTH_EDGES;

	chip = gpiod_line_get_chip(line);
	fd = chip_get_fd(chip);;

	status = ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, req);
	if (status < 0)
		return -1;

	line_set_state(line, LINE_EVENT);

	return 0;
}

static int line_event_request_type(struct gpiod_line *line,
				   const char *consumer,
				   bool active_low, int type)
{
	struct gpiod_line_evreq_config config = {
		.consumer = consumer,
		.event_type = type,
		.active_state = active_low ? GPIOD_ACTIVE_STATE_LOW
					   : GPIOD_ACTIVE_STATE_HIGH,
	};

	return gpiod_line_event_request(line, &config);
}

int gpiod_line_event_request_rising(struct gpiod_line *line,
				    const char *consumer, bool active_low)
{
	return line_event_request_type(line, consumer, active_low,
				       GPIOD_EVENT_RISING_EDGE);
}

int gpiod_line_event_request_falling(struct gpiod_line *line,
				     const char *consumer, bool active_low)
{
	return line_event_request_type(line, consumer, active_low,
				       GPIOD_EVENT_FALLING_EDGE);
}

int gpiod_line_event_request_all(struct gpiod_line *line,
				 const char *consumer, bool active_low)
{
	return line_event_request_type(line, consumer, active_low,
				       GPIOD_EVENT_BOTH_EDGES);
}

bool gpiod_line_event_configured(struct gpiod_line *line)
{
	return line_get_state(line) == LINE_EVENT;
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
			       struct gpiod_line **line)
{
	struct pollfd fds[GPIOD_REQUEST_MAX_LINES];
	struct gpiod_line *linetmp;
	unsigned int i;
	int status;

	if (!line_bulk_is_event_configured(bulk)) {
		errno = EPERM;
		return -1;
	}

	memset(fds, 0, sizeof(fds));

	for (i = 0; i < bulk->num_lines; i++) {
		linetmp = bulk->lines[i];

		fds[i].fd = line_get_event_fd(linetmp);
		fds[i].events = POLLIN | POLLPRI;
	}

	status = ppoll(fds, bulk->num_lines, timeout, NULL);
	if (status < 0)
		return -1;
	else if (status == 0)
		return 0;

	for (i = 0; !fds[i].revents; i++);
	if (line)
		*line = bulk->lines[i];

	return 1;
}

int gpiod_line_event_read(struct gpiod_line *line,
			  struct gpiod_line_event *event)
{
	int fd;

	if (!gpiod_line_event_configured(line)) {
		errno = EPERM;
		return -1;
	}

	fd = line_get_event_fd(line);

	return gpiod_line_event_read_fd(fd, event);
}

int gpiod_line_event_get_fd(struct gpiod_line *line)
{
	return line_get_state(line) == LINE_EVENT
				? line_get_event_fd(line) : -1;
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
						? GPIOD_EVENT_RISING_EDGE
						: GPIOD_EVENT_FALLING_EDGE;

	event->ts.tv_sec = evdata.timestamp / 1000000000ULL;
	event->ts.tv_nsec = evdata.timestamp % 1000000000ULL;

	return 0;
}
