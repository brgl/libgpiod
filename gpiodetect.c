/*
 * List all GPIO chips.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */

#include <gpiod.h>

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
	struct gpiod_chip_iter *iter;
	struct gpiod_chip *chip;
	int status;

	if (argc != 1) {
		printf("Usage: %s\n", argv[0]);
		printf("List all GPIO chips\n");

		return EXIT_FAILURE;
	}

	iter = gpiod_chip_iter_new();
	if (GPIOD_IS_ERR(iter)) {
		status = GPIOD_PTR_ERR(iter);
		goto err;
	}

	gpiod_foreach_chip(iter, chip) {
		printf("%s [%s] (%u lines)\n",
		       gpiod_chip_name(chip),
		       gpiod_chip_label(chip),
		       gpiod_chip_num_lines(chip));
	}

	gpiod_chip_iter_free(iter);

	if (GPIOD_IS_ERR(chip)) {
		status = GPIOD_PTR_ERR(chip);
		goto err;
	}

	return EXIT_SUCCESS;

err:
	fprintf(stderr, "%s: unable to access gpio chips: %s\n",
		argv[0], strerror(-status));

	return EXIT_FAILURE;
}
