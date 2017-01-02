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
	return strerror(errnum);
}

int gpiod_simple_get_value(const char *device, unsigned int offset)
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

	status = gpiod_line_request(line, libgpiod_consumer,
				    GPIOD_DIRECTION_IN, 0, 0);
	if (status < 0) {
		gpiod_chip_close(chip);
		return -1;
	}

	value = gpiod_line_get_value(line);

	gpiod_line_release(line);
	gpiod_chip_close(chip);

	return value;
}

int gpiod_simple_set_value(const char *device, unsigned int offset, int value)
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

	status = gpiod_line_request(line, libgpiod_consumer,
				    GPIOD_DIRECTION_OUT, value, 0);
	if (status < 0) {
		gpiod_chip_close(chip);
		return -1;
	}

	gpiod_line_release(line);
	gpiod_chip_close(chip);

	return status;
}

struct gpiod_line {
	bool requested;
	struct gpiod_chip *chip;
	struct gpioline_info info;
	struct gpiohandle_request *req;
};

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
	return line->info.flags & GPIOLINE_FLAG_IS_OUT ? GPIOD_DIRECTION_OUT
							: GPIOD_DIRECTION_IN;
}

bool gpiod_line_is_active_low(struct gpiod_line *line)
{
	return line->info.flags & GPIOLINE_FLAG_ACTIVE_LOW;
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

int gpiod_line_request(struct gpiod_line *line, const char *consumer,
		       int direction, int default_val, int flags)
{
	struct gpiohandle_request *req;
	struct gpiod_chip *chip;
	int status, fd;

	req = zalloc(sizeof(*req));
	if (!req)
		return -1;

	line->req = req;
	memset(req, 0, sizeof(*req));

	if (flags & GPIOD_REQUEST_ACTIVE_LOW)
		req->flags |= GPIOHANDLE_REQUEST_ACTIVE_LOW;
	if (flags & GPIOD_REQUEST_OPEN_DRAIN)
		req->flags |= GPIOHANDLE_REQUEST_OPEN_DRAIN;
	if (flags & GPIOD_REQUEST_OPEN_SOURCE)
		req->flags |= GPIOHANDLE_REQUEST_OPEN_SOURCE;

	req->flags |= direction == GPIOD_DIRECTION_IN
					? GPIOHANDLE_REQUEST_INPUT
					: GPIOHANDLE_REQUEST_OUTPUT;

	req->lineoffsets[0] = line->info.line_offset;
	req->lines = 1;

	if (direction == GPIOD_DIRECTION_OUT)
		req->default_values[0] = (__u8)default_val;

	strncpy(req->consumer_label, consumer,
		sizeof(req->consumer_label) - 1);

	chip = gpiod_line_get_chip(line);
	fd = gpiod_chip_get_fd(chip);

	status = gpio_ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, req);
	if (status < 0)
		return -1;

	line->requested = true;

	return 0;
}

void gpiod_line_release(struct gpiod_line *line)
{
	close(line->req->fd);
	free(line->req);
	line->requested = false;
}

bool gpiod_line_is_requested(struct gpiod_line *line)
{
	return line->requested;
}

int gpiod_line_get_value(struct gpiod_line *line)
{
	struct gpiohandle_data data;
	int status;

	if (!gpiod_line_is_requested(line)) {
		set_last_error(EPERM);
		return -1;
	}

	memset(&data, 0, sizeof(data));

	status = gpio_ioctl(line->req->fd,
			    GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
	if (status < 0)
		return -1;

	return data.values[0];
}

int gpiod_line_set_value(struct gpiod_line *line, int value)
{
	struct gpiohandle_data data;
	int status;

	if (!gpiod_line_is_requested(line)) {
		set_last_error(EPERM);
		return -1;
	}

	memset(&data, 0, sizeof(data));
	data.values[0] = value ? 1 : 0;

	status = gpio_ioctl(line->req->fd,
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

	memset(&line->info, 0, sizeof(line->info));
	line->info.line_offset = offset;

	status = gpio_ioctl(chip->fd, GPIO_GET_LINEINFO_IOCTL, &line->info);
	if (status < 0)
		return NULL;

	line->chip = chip;

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
			iter->current = gpiod_chip_open(dentry->d_name);
			return iter->current;
		}
	}

	return NULL;
}
