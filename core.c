/*
 * GPIO chardev utils for linux.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */

#include "gpiod.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <linux/gpio.h>

struct gpiod_chip
{
	int fd;
	struct gpiochip_info cinfo;
	struct gpiod_line *lines;
};

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

enum {
	CHIP_ITER_INIT = 0,
	CHIP_ITER_DONE,
	CHIP_ITER_ERR,
};

struct gpiod_chip_iter
{
	DIR *dir;
	struct gpiod_chip *current;
	int state;
	char *failed_chip;
};

static const char dev_dir[] = "/dev/";
static const char cdev_prefix[] = "gpiochip";
static const char libgpiod_consumer[] = "libgpiod";

/*
 * The longest error message in glibc is about 50 characters long so 64 should
 * be enough to store every error message in the future too.
 */
#define ERRSTR_MAX 64

static __thread int last_error;
static __thread char errmsg[ERRSTR_MAX];

static const char *const error_descr[] = {
	"success",
	"GPIO line not reserved",
	"no events configured on GPIO line",
	"GPIO lines in bulk don't belong to the same gpiochip",
	"GPIO line currently in use",
};

static void set_last_error(int errnum)
{
	last_error = errnum;
}

static void last_error_from_errno(void)
{
	last_error = errno;
}

static void * zalloc(size_t size)
{
	void *ptr;

	ptr = malloc(size);
	if (!ptr) {
		set_last_error(ENOMEM);
		return NULL;
	}

	memset(ptr, 0, size);

	return ptr;
}

static bool is_unsigned_int(const char *str)
{
	for (; *str && isdigit(*str); str++);

	return *str == '\0';
}

static void nsec_to_timespec(uint64_t nsec, struct timespec *ts)
{
	ts->tv_sec = nsec / 1000000000ULL;
	ts->tv_nsec = (nsec % 1000000000ULL);
}

static int gpio_ioctl(int fd, unsigned long request, void *data)
{
	int status;

	status = ioctl(fd, request, data);
	if (status < 0) {
		last_error_from_errno();
		return -1;
	}

	return 0;
}

int gpiod_errno(void)
{
	return last_error;
}

const char * gpiod_strerror(int errnum)
{
	if (errnum < __GPIOD_ERRNO_OFFSET)
		return strerror_r(errnum, errmsg, sizeof(errmsg));
	else if (errnum > __GPIOD_MAX_ERR)
		return "invalid error number";
	else
		return error_descr[errnum - __GPIOD_ERRNO_OFFSET];
}

const char * gpiod_last_strerror(void)
{
	return gpiod_strerror(gpiod_errno());
}

int gpiod_simple_get_value(const char *device,
			   unsigned int offset, bool active_low)
{
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int status, value;

	chip = gpiod_chip_open_lookup(device);
	if (!chip)
		return -1;

	line = gpiod_chip_get_line(chip, offset);
	if (!line) {
		gpiod_chip_close(chip);
		return -1;
	}

	status = gpiod_line_request_input(line, libgpiod_consumer, active_low);
	if (status < 0) {
		gpiod_chip_close(chip);
		return -1;
	}

	value = gpiod_line_get_value(line);

	gpiod_line_release(line);
	gpiod_chip_close(chip);

	return value;
}

int gpiod_simple_set_value(const char *device, unsigned int offset, int value,
			   bool active_low, void (*cb)(void *), void *data)
{
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int status;

	chip = gpiod_chip_open_lookup(device);
	if (!chip)
		return -1;

	line = gpiod_chip_get_line(chip, offset);
	if (!line) {
		gpiod_chip_close(chip);
		return -1;
	}

	status = gpiod_line_request_output(line, libgpiod_consumer,
					   active_low, value);
	if (status < 0) {
		gpiod_chip_close(chip);
		return -1;
	}

	if (cb)
		cb(data);

	gpiod_line_release(line);
	gpiod_chip_close(chip);

	return 0;
}

int gpiod_simple_event_loop(const char *device, unsigned int offset,
			    bool active_low, struct timespec *timeout,
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

	status = gpiod_line_event_request_all(line,
					      libgpiod_consumer, active_low);
	if (status < 0) {
		gpiod_chip_close(chip);
		return -1;
	}

	for (;;) {
		status = gpiod_line_event_wait(line, timeout);
		if (status < 0) {
			if (gpiod_errno() == EINTR)
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
		if (status == GPIOD_EVENT_CB_STOP)
			goto out;
	}

out:
	gpiod_line_event_release(line);
	gpiod_chip_close(chip);

	return status;
}

static void line_set_offset(struct gpiod_line *line, unsigned int offset)
{
	line->info.line_offset = offset;
}

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
	if (line_get_state(line) != LINE_TAKEN)
		return -1;
	else
		return line->handle->request.fd;
}

static int line_get_event_fd(struct gpiod_line *line)
{
	if (line_get_state(line) != LINE_EVENT)
		return -1;
	else
		return line->event.fd;
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

static void line_update(struct gpiod_line *line)
{
	int status;

	status = gpiod_line_update(line);
	if (status < 0)
		line_set_needs_update(line);
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
	fd = chip->fd;

	status = gpio_ioctl(fd, GPIO_GET_LINEINFO_IOCTL, &line->info);
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

static bool verify_line_bulk(struct gpiod_line_bulk *line_bulk)
{
	struct gpiod_chip *chip;
	unsigned int i;

	chip = gpiod_line_get_chip(line_bulk->lines[0]);

	for (i = 1; i < line_bulk->num_lines; i++) {
		if (chip != gpiod_line_get_chip(line_bulk->lines[i])) {
			set_last_error(GPIOD_EBULKINCOH);
			return false;
		}

		if (!gpiod_line_is_free(line_bulk->lines[i])) {
			set_last_error(GPIOD_ELINEBUSY);
			return false;
		}
	}

	return true;
}

int gpiod_line_request_bulk(struct gpiod_line_bulk *bulk,
			    const struct gpiod_line_request_config *config,
			    int *default_vals)
{
	struct gpiohandle_request *req;
	struct handle_data *handle;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int status, fd;
	unsigned int i;

	if (!verify_line_bulk(bulk))
		return -1;

	handle = zalloc(sizeof(*handle));
	if (!handle)
		return -1;

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
	fd = chip->fd;

	status = gpio_ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, req);
	if (status < 0)
		return -1;

	for (i = 0; i < bulk->num_lines; i++) {
		line = bulk->lines[i];

		line_set_handle(line, handle);
		line_set_state(line, LINE_TAKEN);
		line_update(line);
	}

	return 0;
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

		line_remove_handle(line);
		line_set_state(line, LINE_FREE);
		line_update(line);
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

static bool line_bulk_is_reserved(struct gpiod_line_bulk *line_bulk)
{
	unsigned int i;

	for (i = 0; i < line_bulk->num_lines; i++) {
		if (!gpiod_line_is_reserved(line_bulk->lines[i]))
			return false;
	}

	return true;
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
	unsigned int i;
	int status;

	if (!line_bulk_is_reserved(bulk)) {
		set_last_error(GPIOD_EREQUEST);
		return -1;
	}

	memset(&data, 0, sizeof(data));

	status = gpio_ioctl(line_get_handle_fd(bulk->lines[0]),
			    GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
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
		set_last_error(GPIOD_EREQUEST);
		return -1;
	}

	memset(&data, 0, sizeof(data));

	for (i = 0; i < bulk->num_lines; i++)
		data.values[i] = (__u8)!!values[i];

	status = gpio_ioctl(line_get_handle_fd(bulk->lines[0]),
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
		gpiod_line_iter_init(&line_iter, chip);
		gpiod_foreach_line(&line_iter, line) {
			line_name = gpiod_line_name(line);
			if (!line_name)
				continue;

			if (strcmp(gpiod_line_name(line), name) == 0) {
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
		set_last_error(GPIOD_ELINEBUSY);
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
		req->eventflags |= GPIOEVENT_EVENT_RISING_EDGE;
	else if (config->event_type == GPIOD_EVENT_FALLING_EDGE)
		req->eventflags |= GPIOEVENT_EVENT_FALLING_EDGE;
	else if (req->eventflags |= GPIOD_EVENT_BOTH_EDGES)
		req->eventflags |= GPIOEVENT_REQUEST_BOTH_EDGES;

	chip = gpiod_line_get_chip(line);
	fd = chip->fd;

	status = gpio_ioctl(fd, GPIO_GET_LINEEVENT_IOCTL, req);
	if (status < 0)
		return -1;

	line_set_state(line, LINE_EVENT);

	return 0;
}

void gpiod_line_event_release(struct gpiod_line *line)
{
	close(line_get_event_fd(line));
	line_set_state(line, LINE_FREE);
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

static bool line_bulk_is_event_configured(struct gpiod_line_bulk *line_bulk)
{
	unsigned int i;

	for (i = 0; i < line_bulk->num_lines; i++) {
		if (!gpiod_line_event_configured(line_bulk->lines[i]))
			return false;
	}

	return true;
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
		set_last_error(GPIOD_EEVREQUEST);
		return -1;
	}

	memset(fds, 0, sizeof(fds));

	for (i = 0; i < bulk->num_lines; i++) {
		linetmp = bulk->lines[i];

		fds[i].fd = line_get_event_fd(linetmp);
		fds[i].events = POLLIN | POLLPRI;
	}

	status = ppoll(fds, bulk->num_lines, timeout, NULL);
	if (status < 0) {
		last_error_from_errno();
		return -1;
	} else if (status == 0) {
		return 0;
	}

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
		set_last_error(GPIOD_EEVREQUEST);
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
		last_error_from_errno();
		return -1;
	} else if (rd != sizeof(evdata)) {
		set_last_error(EIO);
		return -1;
	}

	event->event_type = evdata.id == GPIOEVENT_EVENT_RISING_EDGE
						? GPIOD_EVENT_RISING_EDGE
						: GPIOD_EVENT_FALLING_EDGE;
	nsec_to_timespec(evdata.timestamp, &event->ts);

	return 0;
}

struct gpiod_chip * gpiod_chip_open(const char *path)
{
	struct gpiod_chip *chip;
	int status, fd;

	fd = open(path, O_RDWR);
	if (fd < 0) {
		last_error_from_errno();
		return NULL;
	}

	chip = zalloc(sizeof(*chip));
	if (!chip) {
		close(fd);
		return NULL;
	}

	chip->fd = fd;

	status = gpio_ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &chip->cinfo);
	if (status < 0) {
		close(chip->fd);
		free(chip);
		return NULL;
	}

	chip->lines = zalloc(chip->cinfo.lines * sizeof(*chip->lines));
	if (!chip->lines) {
		close(chip->fd);
		free(chip);
		return NULL;
	}

	return chip;
}

struct gpiod_chip * gpiod_chip_open_by_name(const char *name)
{
	struct gpiod_chip *chip;
	char *path;
	int status;

	status = asprintf(&path, "%s%s", dev_dir, name);
	if (status < 0) {
		last_error_from_errno();
		return NULL;
	}

	chip = gpiod_chip_open(path);
	free(path);

	return chip;
}

struct gpiod_chip * gpiod_chip_open_by_number(unsigned int num)
{
	struct gpiod_chip *chip;
	char *path;
	int status;

	status = asprintf(&path, "%s%s%u", dev_dir, cdev_prefix, num);
	if (!status) {
		last_error_from_errno();
		return NULL;
	}

	chip = gpiod_chip_open(path);
	free(path);

	return chip;
}

struct gpiod_chip * gpiod_chip_open_lookup(const char *descr)
{
	if (is_unsigned_int(descr))
		return gpiod_chip_open_by_number(strtoul(descr, NULL, 10));
	else if (strncmp(descr, dev_dir, sizeof(dev_dir) - 1) != 0)
		return gpiod_chip_open_by_name(descr);
	else
		return gpiod_chip_open(descr);
}

void gpiod_chip_close(struct gpiod_chip *chip)
{
	unsigned int i;

	for (i = 0; i < chip->cinfo.lines; i++) {
		if (chip->lines[i].state == LINE_TAKEN)
			gpiod_line_release(&chip->lines[i]);
		else if (chip->lines[i].state == LINE_EVENT)
			gpiod_line_event_release(&chip->lines[i]);
	}

	close(chip->fd);
	free(chip->lines);
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
		set_last_error(EINVAL);
		return NULL;
	}

	line = &chip->lines[offset];
	line_set_offset(line, offset);
	line->chip = chip;

	status = gpiod_line_update(line);
	if (status < 0)
		return NULL;

	return line;
}

struct gpiod_chip * gpiod_line_get_chip(struct gpiod_line *line)
{
	return line->chip;
}

struct gpiod_chip_iter * gpiod_chip_iter_new(void)
{
	struct gpiod_chip_iter *new;

	new = zalloc(sizeof(*new));
	if (!new)
		return NULL;

	new->dir = opendir(dev_dir);
	if (!new->dir) {
		last_error_from_errno();
		return NULL;
	}

	new->state = CHIP_ITER_INIT;

	return new;
}

void gpiod_chip_iter_free(struct gpiod_chip_iter *iter)
{
	if (iter->current)
		gpiod_chip_close(iter->current);

	gpiod_chip_iter_free_noclose(iter);
}

void gpiod_chip_iter_free_noclose(struct gpiod_chip_iter *iter)
{
	closedir(iter->dir);
	if (iter->failed_chip)
		free(iter->failed_chip);
	free(iter);
}

struct gpiod_chip * gpiod_chip_iter_next(struct gpiod_chip_iter *iter)
{
	struct gpiod_chip *chip;
	struct dirent *dentry;
	int status;

	if (iter->current) {
		gpiod_chip_close(iter->current);
		iter->current = NULL;
	}

	for (dentry = readdir(iter->dir);
	     dentry;
	     dentry = readdir(iter->dir)) {
		status = strncmp(dentry->d_name, cdev_prefix,
				 sizeof(cdev_prefix) - 1);
		if (status == 0) {
			iter->state = CHIP_ITER_INIT;
			if (iter->failed_chip) {
				free(iter->failed_chip);
				iter->failed_chip = NULL;
			}

			chip = gpiod_chip_open_by_name(dentry->d_name);
			if (!chip) {
				iter->state = CHIP_ITER_ERR;
				iter->failed_chip = strdup(dentry->d_name);
				/* No point in an error check here. */
			}

			iter->current = chip;
			return iter->current;
		}
	}

	iter->state = CHIP_ITER_DONE;
	return NULL;
}

bool gpiod_chip_iter_done(struct gpiod_chip_iter *iter)
{
	return iter->state == CHIP_ITER_DONE;
}

bool gpiod_chip_iter_iserr(struct gpiod_chip_iter *iter)
{
	return iter->state == CHIP_ITER_ERR;
}

const char *
gpiod_chip_iter_failed_chip(struct gpiod_chip_iter *iter)
{
	return iter->failed_chip;
}

struct gpiod_line * gpiod_line_iter_next(struct gpiod_line_iter *iter)
{
	struct gpiod_line *line;

	if (iter->offset >= gpiod_chip_num_lines(iter->chip)) {
		iter->state = GPIOD_LINE_ITER_DONE;
		return NULL;
	}

	iter->state = GPIOD_LINE_ITER_INIT;
	line = gpiod_chip_get_line(iter->chip, iter->offset++);
	if (!line)
		iter->state = GPIOD_LINE_ITER_ERR;

	return line;
}

const char * gpiod_version_string(void)
{
	return PACKAGE_VERSION;
}
