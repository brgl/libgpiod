/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include "line.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

struct gpiod_chip {
	int fd;
	struct gpiochip_info cinfo;
	struct gpiod_line **lines;
	struct line_chip_ctx *chip_ctx;
};

static bool isuint(const char *str)
{
	for (; *str && isdigit(*str); str++);

	return *str == '\0';
}

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

struct gpiod_chip * gpiod_chip_open_by_name(const char *name)
{
	struct gpiod_chip *chip;
	char *path;
	int status;

	status = asprintf(&path, "/dev/%s", name);
	if (status < 0)
		return NULL;

	chip = gpiod_chip_open(path);
	free(path);

	return chip;
}

struct gpiod_chip * gpiod_chip_open_by_number(unsigned int num)
{
	struct gpiod_chip *chip;
	char *path;
	int status;

	status = asprintf(&path, "/dev/gpiochip%u", num);
	if (!status)
		return NULL;

	chip = gpiod_chip_open(path);
	free(path);

	return chip;
}

struct gpiod_chip * gpiod_chip_open_by_label(const char *label)
{
	struct gpiod_chip_iter *iter;
	struct gpiod_chip *chip;

	iter = gpiod_chip_iter_new();
	if (!iter)
		return NULL;

	gpiod_foreach_chip(iter, chip) {
		if (gpiod_chip_iter_err(iter))
			goto out;

		if (strcmp(label, gpiod_chip_label(chip)) == 0) {
			gpiod_chip_iter_free_noclose(iter);
			return chip;
		}
	}

out:
	gpiod_chip_iter_free(iter);
	return  NULL;
}

struct gpiod_chip * gpiod_chip_open_lookup(const char *descr)
{
	struct gpiod_chip *chip;

	if (isuint(descr)) {
		chip = gpiod_chip_open_by_number(strtoul(descr, NULL, 10));
	} else {
		chip = gpiod_chip_open_by_label(descr);
		if (!chip) {
			if (strncmp(descr, "/dev/", 5))
				chip = gpiod_chip_open_by_name(descr);
			else
				chip = gpiod_chip_open(descr);
		}
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
				line_free(line);
			}
		}

		free(chip->lines);
	}

	if (chip->chip_ctx)
		line_chip_ctx_free(chip->chip_ctx);

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

	if (!chip->chip_ctx) {
		chip->chip_ctx = line_chip_ctx_new(chip, chip->fd);
		if (!chip->chip_ctx)
			return NULL;
	}

	if (!chip->lines[offset]) {
		line = line_new(offset, chip->chip_ctx);
		if (!line)
			return NULL;

		chip->lines[offset] = line;
	} else {
		line = chip->lines[offset];
	}

	status = gpiod_line_update(line);
	if (status < 0)
		return NULL;

	return line;
}

