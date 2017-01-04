/*
 * GPIO chardev utils for linux.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */

#include <gpiod.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

static const char dev_dir[] = "/dev/";
static const char cdev_prefix[] = "gpiochip";
static const char libgpiod_consumer[] = "libgpiod";

static __thread int last_error;

static const char *const error_descr[] = {
	"success",
	"GPIO line not requested",
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
		return strerror(errnum);
	else if (errnum > __GPIOD_MAX_ERR)
		return "invalid error number";
	else
		return error_descr[errnum - __GPIOD_ERRNO_OFFSET];
}

int gpiod_simple_get_value(const char *device, unsigned int offset)
{
	struct gpiod_line_request_config config;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int status, value;

	memset(&config, 0, sizeof(config));
	config.consumer = libgpiod_consumer;
	config.direction = GPIOD_DIRECTION_AS_IS;

	chip = gpiod_chip_open_lookup(device);
	if (!chip)
		return -1;

	line = gpiod_chip_get_line(chip, offset);
	if (!line) {
		gpiod_chip_close(chip);
		return -1;
	}

	status = gpiod_line_request(line, &config, 0);
	if (status < 0) {
		gpiod_chip_close(chip);
		return -1;
	}

	value = gpiod_line_get_value(line);

	gpiod_line_release(line);
	gpiod_chip_close(chip);

	return value;
}

struct gpiod_line {
	bool requested;
	bool up_to_date;
	struct gpiod_chip *chip;
	struct gpioline_info info;
	struct gpiohandle_request *req;
};

static void line_set_offset(struct gpiod_line *line, unsigned int offset)
{
	line->info.line_offset = offset;
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

int gpiod_line_polarity(struct gpiod_line *line)
{
	return line->info.flags & GPIOLINE_FLAG_ACTIVE_LOW
					? GPIOD_POLARITY_ACTIVE_LOW
					: GPIOD_POLARITY_ACTIVE_HIGH;
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

bool gpiod_line_needs_update(struct gpiod_line *line)
{
	return !line->up_to_date;
}

int gpiod_line_update(struct gpiod_line *line)
{
	struct gpiod_chip *chip;
	int status, fd;

	memset(&line->info, 0, sizeof(line->info));

	chip = gpiod_line_get_chip(line);
	fd = gpiod_chip_get_fd(chip);

	status = gpio_ioctl(fd, GPIO_GET_LINEINFO_IOCTL, &line->info);
	if (status < 0)
		return -1;

	line->up_to_date = true;

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
		if (chip != gpiod_line_get_chip(line_bulk->lines[i]))
			return false;
	}

	return true;
}

int gpiod_line_request_bulk(struct gpiod_line_bulk *line_bulk,
			    const struct gpiod_line_request_config *config,
			    int *default_vals)
{
	struct gpiohandle_request *req;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int status, fd;
	unsigned int i;

	/* Paranoia: verify that all lines are from the same gpiochip. */
	if (!verify_line_bulk(line_bulk))
		return -1;

	req = zalloc(sizeof(*req));
	if (!req)
		return -1;

	if (config->flags & GPIOD_REQUEST_OPEN_DRAIN)
		req->flags |= GPIOHANDLE_REQUEST_OPEN_DRAIN;
	if (config->flags & GPIOD_REQUEST_OPEN_SOURCE)
		req->flags |= GPIOHANDLE_REQUEST_OPEN_SOURCE;

	if (config->direction == GPIOD_DIRECTION_INPUT)
		req->flags |= GPIOHANDLE_REQUEST_INPUT;
	else if (config->direction == GPIOD_DIRECTION_OUTPUT)
		req->flags |= GPIOHANDLE_REQUEST_OUTPUT;

	if (config->polarity == GPIOD_POLARITY_ACTIVE_LOW)
		req->flags |= GPIOHANDLE_REQUEST_ACTIVE_LOW;

	req->lines = line_bulk->num_lines;

	for (i = 0; i < line_bulk->num_lines; i++) {
		req->lineoffsets[i] = gpiod_line_offset(line_bulk->lines[i]);
		if (config->direction == GPIOD_DIRECTION_OUTPUT)
			req->default_values[i] = !!default_vals[i];
	}

	strncpy(req->consumer_label, config->consumer,
		sizeof(req->consumer_label) - 1);

	chip = gpiod_line_get_chip(line_bulk->lines[0]);
	fd = gpiod_chip_get_fd(chip);

	status = gpio_ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, req);
	if (status < 0)
		return -1;

	for (i = 0; i < line_bulk->num_lines; i++) {
		line = line_bulk->lines[i];

		line->req = req;
		line->requested = true;
		/*
		 * Update line info to include the changes after the
		 * request.
		 */
		status = gpiod_line_update(line);
		if (status < 0)
			line->up_to_date = false;
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

void gpiod_line_release_bulk(struct gpiod_line_bulk *line_bulk)
{
	struct gpiod_line *line;
	unsigned int i;
	int status;

	close(line_bulk->lines[0]->req->fd);
	free(line_bulk->lines[0]->req);

	for (i = 0; i < line_bulk->num_lines; i++) {
		line = line_bulk->lines[i];

		line->req = NULL;
		line->requested = false;

		status = gpiod_line_update(line);
		if (status < 0)
			line->up_to_date = false;
	}
}

bool gpiod_line_is_requested(struct gpiod_line *line)
{
	return line->requested;
}

static bool line_bulk_is_requested(struct gpiod_line_bulk *line_bulk)
{
	unsigned int i;

	for (i = 0; i < line_bulk->num_lines; i++) {
		if (!gpiod_line_is_requested(line_bulk->lines[i]))
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

int gpiod_line_get_value_bulk(struct gpiod_line_bulk *line_bulk, int *values)
{
	struct gpiohandle_data data;
	unsigned int i;
	int status;

	if (!line_bulk_is_requested(line_bulk)) {
		set_last_error(GPIOD_ENOTREQUESTED);
		return -1;
	}

	memset(&data, 0, sizeof(data));

	status = gpio_ioctl(line_bulk->lines[0]->req->fd,
			    GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
	if (status < 0)
		return -1;

	for (i = 0; i < line_bulk->num_lines; i++)
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

int gpiod_line_set_value_bulk(struct gpiod_line_bulk *line_bulk, int *values)
{
	struct gpiohandle_data data;
	unsigned int i;
	int status;

	if (!line_bulk_is_requested(line_bulk)) {
		set_last_error(GPIOD_ENOTREQUESTED);
		return -1;
	}

	memset(&data, 0, sizeof(data));

	for (i = 0; i < line_bulk->num_lines; i++)
		data.values[i] = (__u8)!!values[i];

	status = gpio_ioctl(line_bulk->lines[0]->req->fd,
			    GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
	if (status < 0)
		return -1;

	return 0;
}

struct gpiod_chip
{
	int fd;
	struct gpiochip_info cinfo;
	struct gpiod_line *lines;
};

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
		if (chip->lines[i].requested)
			gpiod_line_release(&chip->lines[i]);
	}

	close(chip->fd);
	free(chip->lines);
	free(chip);
}

const char * gpiod_chip_name(struct gpiod_chip *chip)
{
	return chip->cinfo.name;
}

const char * gpiod_chip_label(struct gpiod_chip *chip)
{
	/* REVISIT can a gpiochip not have a label? */
	return chip->cinfo.label;
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

int gpiod_chip_get_fd(struct gpiod_chip *chip)
{
	return chip->fd;
}

struct gpiod_chip * gpiod_line_get_chip(struct gpiod_line *line)
{
	return line->chip;
}

struct gpiod_chip_iter
{
	DIR *dir;
	struct gpiod_chip *current;
};

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

	return new;
}

void gpiod_chip_iter_free(struct gpiod_chip_iter *iter)
{
	if (iter->current)
		gpiod_chip_close(iter->current);

	closedir(iter->dir);
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
			chip = gpiod_chip_open_by_name(dentry->d_name);
			iter->current = chip;
			return iter->current;
		}
	}

	return NULL;
}

struct gpiod_line * gpiod_line_find_by_name(const char *name)
{
	struct gpiod_chip_iter *chip_iter;
	struct gpiod_line_iter line_iter;
	struct gpiod_chip *chip;
	struct gpiod_line *line;

	chip_iter = gpiod_chip_iter_new();
	if (!chip_iter)
		return NULL;

	gpiod_foreach_chip(chip_iter, chip) {
		gpiod_line_iter_init(&line_iter, chip);
		gpiod_foreach_line(&line_iter, line) {
			if (strcmp(gpiod_line_name(line), name) == 0) {
				/* TODO A separate function for that maybe? */
				closedir(chip_iter->dir);
				free(chip_iter);
				return line;
			}
		}
	}

	gpiod_chip_iter_free(chip_iter);

	return NULL;
}
