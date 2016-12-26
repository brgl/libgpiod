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

static void * zalloc(size_t size)
{
	void *ptr;

	ptr = malloc(size);
	if (ptr)
		memset(ptr, 0, size);

	return ptr;
}

static bool is_unsigned_int(const char *str)
{
	for (; *str && isdigit(*str); str++);

	return *str == '\0';
}

int gpiod_simple_get_value(const char *device, unsigned int offset)
{
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	int status, value;

	chip = gpiod_chip_open_lookup(device);
	if (GPIOD_IS_ERR(chip))
		return GPIOD_PTR_ERR(chip);

	line = gpiod_chip_get_line(chip, offset);
	if (GPIOD_IS_ERR(line)) {
		gpiod_chip_close(chip);
		return GPIOD_PTR_ERR(line);
	}

	status = gpiod_line_request(line, libgpiod_consumer,
				    GPIOD_DIRECTION_IN, 0, 0);
	if (status < 0) {
		gpiod_chip_close(chip);
		return status;
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
	if (GPIOD_IS_ERR(chip))
		return GPIOD_PTR_ERR(chip);

	line = gpiod_chip_get_line(chip, offset);
	if (GPIOD_IS_ERR(line)) {
		gpiod_chip_close(chip);
		return GPIOD_PTR_ERR(line);
	}

	status = gpiod_line_request(line, libgpiod_consumer,
				    GPIOD_DIRECTION_OUT, value, 0);
	if (status < 0) {
		gpiod_chip_close(chip);
		return status;
	}

	gpiod_line_release(line);
	gpiod_chip_close(chip);

	return status;
}

struct gpiod_line {
	bool requested;
	struct gpiod_chip *chip;
	struct gpioline_info linfo;
	struct gpiohandle_request lreq;
};

unsigned int gpiod_line_offset(struct gpiod_line *line)
{
	return (unsigned int)line->linfo.line_offset;
}

const char * gpiod_line_name(struct gpiod_line *line)
{
	return line->linfo.name[0] == '\0' ? NULL : line->linfo.name;
}

const char * gpiod_line_consumer(struct gpiod_line *line)
{
	return line->linfo.consumer[0] == '\0' ? NULL : line->linfo.consumer;
}

int gpiod_line_direction(struct gpiod_line *line)
{
	return line->linfo.flags & GPIOLINE_FLAG_IS_OUT ? GPIOD_DIRECTION_OUT
							: GPIOD_DIRECTION_IN;
}

bool gpiod_line_is_active_low(struct gpiod_line *line)
{
	return line->linfo.flags & GPIOLINE_FLAG_ACTIVE_LOW;
}

bool gpiod_line_is_used_by_kernel(struct gpiod_line *line)
{
	return line->linfo.flags & GPIOLINE_FLAG_KERNEL;
}

bool gpiod_line_is_open_drain(struct gpiod_line *line)
{
	return line->linfo.flags & GPIOLINE_FLAG_OPEN_DRAIN;
}

bool gpiod_line_is_open_source(struct gpiod_line *line)
{
	return line->linfo.flags & GPIOLINE_FLAG_OPEN_SOURCE;
}

int gpiod_line_request(struct gpiod_line *line, const char *consumer,
		       int direction, int default_val, int flags)
{
	struct gpiohandle_request *req;
	struct gpiod_chip *chip;
	int status, fd;

	req = &line->lreq;
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

	req->lineoffsets[0] = line->linfo.line_offset;
	req->lines = 1;
	/* FIXME This doesn't seem to work... */
	if (flags & GPIOD_DIRECTION_OUT)
		req->default_values[0] = default_val ? 1 : 0;

	strncpy(req->consumer_label, consumer,
		sizeof(req->consumer_label) - 1);

	chip = gpiod_line_get_chip(line);
	fd = gpiod_chip_get_fd(chip);

	status = ioctl(fd, GPIO_GET_LINEHANDLE_IOCTL, req);
	if (status < 0)
		return -errno;

	line->requested = true;

	return 0;
}

void gpiod_line_release(struct gpiod_line *line)
{
	close(line->lreq.fd);
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

	if (!gpiod_line_is_requested(line))
		return -EPERM;

	memset(&data, 0, sizeof(data));

	status = ioctl(line->lreq.fd, GPIOHANDLE_GET_LINE_VALUES_IOCTL, &data);
	if (status < 0)
		return -errno;

	return data.values[0];
}

int gpiod_line_set_value(struct gpiod_line *line, int value)
{
	struct gpiohandle_data data;
	int status;

	if (!gpiod_line_is_requested(line))
		return -EPERM;

	memset(&data, 0, sizeof(data));
	data.values[0] = value ? 1 : 0;

	status = ioctl(line->lreq.fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
	if (status < 0)
		return -errno;

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
	if (fd < 0)
		return GPIOD_ERR_PTR(-errno);

	chip = zalloc(sizeof(*chip));
	if (!chip) {
		close(fd);
		return GPIOD_ERR_PTR(-ENOMEM);
	}

	chip->fd = fd;

	status = ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &chip->cinfo);
	if (status < 0) {
		close(chip->fd);
		free(chip);
		return GPIOD_ERR_PTR(-errno);
	}

	chip->lines = zalloc(chip->cinfo.lines * sizeof(*chip->lines));
	if (!chip->lines) {
		close(chip->fd);
		free(chip);
		return GPIOD_ERR_PTR(-ENOMEM);
	}

	return chip;
}

struct gpiod_chip * gpiod_chip_open_by_name(const char *name)
{
	struct gpiod_chip *chip;
	char *path;
	int status;

	status = asprintf(&path, "%s%s", dev_dir, name);
	if (status < 0)
		return GPIOD_ERR_PTR(-errno);

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
	if (!status)
		return GPIOD_ERR_PTR(-ENOMEM);

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

	if (offset >= chip->cinfo.lines)
		return GPIOD_ERR_PTR(-EINVAL);

	line = &chip->lines[offset];

	memset(&line->linfo, 0, sizeof(line->linfo));
	line->linfo.line_offset = offset;

	status = ioctl(chip->fd, GPIO_GET_LINEINFO_IOCTL, &line->linfo);
	if (status < 0)
		return GPIOD_ERR_PTR(-errno);

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
		return GPIOD_ERR_PTR(-ENOMEM);

	new->dir = opendir(dev_dir);
	if (!new->dir)
		return GPIOD_ERR_PTR(-errno);

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
