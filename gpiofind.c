/*
 * Read value from GPIO line.
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
	struct gpiod_line *line;
	struct gpiod_chip *chip;

	if (argc < 2) {
		fprintf(stderr, "%s: gpiochip must be specified\n", argv[0]);
		return EXIT_FAILURE;
	}

	line = gpiod_line_find_by_name(argv[1]);
	if (!line)
		return EXIT_FAILURE;

	chip = gpiod_line_get_chip(line);

	printf("%s %u\n", gpiod_chip_name(chip), gpiod_line_offset(line));

	gpiod_chip_close(chip);

	return EXIT_SUCCESS;
}
