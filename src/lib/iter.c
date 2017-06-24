/*
 * GPIO chip and line iterators for libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include <gpiod.h>

#include <string.h>
#include <dirent.h>

enum {
	CHIP_ITER_INIT = 0,
	CHIP_ITER_DONE,
	CHIP_ITER_ERR,
};

struct gpiod_chip_iter {
	DIR *dir;
	struct gpiod_chip *current;
	int state;
	char *failed_chip;
};

struct gpiod_chip_iter * gpiod_chip_iter_new(void)
{
	struct gpiod_chip_iter *new;

	new = malloc(sizeof(*new));
	if (!new)
		return NULL;

	memset(new, 0, sizeof(*new));

	new->dir = opendir("/dev");
	if (!new->dir)
		return NULL;

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
	if (iter->current) {
		gpiod_chip_close(iter->current);
		iter->current = NULL;
	}

	return gpiod_chip_iter_next_noclose(iter);
}

struct gpiod_chip * gpiod_chip_iter_next_noclose(struct gpiod_chip_iter *iter)
{
	struct gpiod_chip *chip;
	struct dirent *dentry;
	int status;

	for (dentry = readdir(iter->dir);
	     dentry;
	     dentry = readdir(iter->dir)) {
		status = strncmp(dentry->d_name, "gpiochip", 8);
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

bool gpiod_chip_iter_err(struct gpiod_chip_iter *iter)
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
