/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

/* GPIO chip and line iterators. */

#include <gpiod.h>

#include <string.h>
#include <dirent.h>

struct gpiod_chip_iter {
	struct gpiod_chip **chips;
	unsigned int num_chips;
	unsigned int offset;
};

struct gpiod_line_iter {
	struct gpiod_line **lines;
	unsigned int num_lines;
	unsigned int offset;
};

static int dir_filter(const struct dirent *dir)
{
	return !strncmp(dir->d_name, "gpiochip", 8);
}

static void free_dirs(struct dirent ***dirs, unsigned int num_dirs)
{
	unsigned int i;

	for (i = 0; i < num_dirs; i++)
		free((*dirs)[i]);
	free(*dirs);
}

struct gpiod_chip_iter * gpiod_chip_iter_new(void)
{
	struct gpiod_chip_iter *iter;
	struct dirent **dirs;
	int i, num_chips;

	num_chips = scandir("/dev", &dirs, dir_filter, alphasort);
	if (num_chips < 0)
		return NULL;

	iter = malloc(sizeof(*iter));
	if (!iter)
		goto err_free_dirs;

	iter->num_chips = num_chips;
	iter->offset = 0;

	if (num_chips == 0) {
		iter->chips = NULL;
		return iter;
	}

	iter->chips = calloc(num_chips, sizeof(*iter->chips));
	if (!iter->chips)
		goto err_free_iter;

	for (i = 0; i < num_chips; i++) {
		iter->chips[i] = gpiod_chip_open_by_name(dirs[i]->d_name);
		if (!iter->chips[i])
			goto err_close_chips;
	}

	free_dirs(&dirs, num_chips);

	return iter;

err_close_chips:
	for (i = 0; i < num_chips; i++) {
		if (iter->chips[i])
			gpiod_chip_close(iter->chips[i]);
	}

	free(iter->chips);

err_free_iter:
	free(iter);

err_free_dirs:
	free_dirs(&dirs, num_chips);

	return NULL;
}

void gpiod_chip_iter_free(struct gpiod_chip_iter *iter)
{
	if (iter->offset > 0 && iter->offset < iter->num_chips)
		gpiod_chip_close(iter->chips[iter->offset - 1]);
	gpiod_chip_iter_free_noclose(iter);
}

void gpiod_chip_iter_free_noclose(struct gpiod_chip_iter *iter)
{
	unsigned int i;

	for (i = iter->offset; i < iter->num_chips; i++) {
		if (iter->chips[i])
			gpiod_chip_close(iter->chips[i]);
	}

	if (iter->chips)
		free(iter->chips);

	free(iter);
}

struct gpiod_chip * gpiod_chip_iter_next(struct gpiod_chip_iter *iter)
{
	if (iter->offset > 0) {
		gpiod_chip_close(iter->chips[iter->offset - 1]);
		iter->chips[iter->offset - 1] = NULL;
	}

	return gpiod_chip_iter_next_noclose(iter);
}

struct gpiod_chip * gpiod_chip_iter_next_noclose(struct gpiod_chip_iter *iter)
{
	return iter->offset < (iter->num_chips)
					? iter->chips[iter->offset++] : NULL;
}

struct gpiod_line_iter * gpiod_line_iter_new(struct gpiod_chip *chip)
{
	struct gpiod_line_iter *iter;
	unsigned int i;

	iter = malloc(sizeof(*iter));
	if (!iter)
		return NULL;

	iter->num_lines = gpiod_chip_num_lines(chip);
	iter->offset = 0;

	iter->lines = calloc(iter->num_lines, sizeof(*iter->lines));
	if (!iter->lines) {
		free(iter);
		return NULL;
	}

	for (i = 0; i < iter->num_lines; i++) {
		iter->lines[i] = gpiod_chip_get_line(chip, i);
		if (!iter->lines[i]) {
			free(iter->lines);
			free(iter);
			return NULL;
		}
	}

	return iter;
}

void gpiod_line_iter_free(struct gpiod_line_iter *iter)
{
	free(iter->lines);
	free(iter);
}

struct gpiod_line * gpiod_line_iter_next(struct gpiod_line_iter *iter)
{
	return iter->offset < (iter->num_lines)
					? iter->lines[iter->offset++] : NULL;
}
