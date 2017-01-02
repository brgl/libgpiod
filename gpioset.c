/*
 * Set value of a GPIO line.
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
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	unsigned int offset;
	char *device, *end;
	int value, status;

	if (argc < 2) {
		fprintf(stderr, "%s: gpiochip must be specified\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (argc < 3) {
		fprintf(stderr,
			"%s: gpio line offset must be specified\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (argc < 4) {
		fprintf(stderr, "%s: value must be specified\n", argv[0]);
		return EXIT_FAILURE;
	}

	device = argv[1];
	/* FIXME Handle negative numbers. */
	offset = strtoul(argv[2], &end, 10);
	if (*end != '\0') {
		fprintf(stderr,
			"%s: invalid GPIO offset: %s\n", argv[0], argv[2]);
		return EXIT_FAILURE;
	}

	value = strtoul(argv[3], &end, 10);
	if (*end != '\0' || (value != 0 && value != 1)) {
		fprintf(stderr, "%s: invalid value: %s\n", argv[0], argv[3]);
		return EXIT_FAILURE;
	}

	chip = gpiod_chip_open_lookup(device);
	if (!chip) {
		fprintf(stderr,
			"%s: error accessing gpiochip %s: %s\n",
			argv[0], device, gpiod_strerror(gpiod_errno()));
		return EXIT_FAILURE;
	}

	line = gpiod_chip_get_line(chip, offset);
	if (!line) {
		fprintf(stderr,
			"%s: error accessing line %u: %s\n",
			argv[0], offset, gpiod_strerror(gpiod_errno()));
		return EXIT_FAILURE;
	}

	status = gpiod_line_request_dout(line, "gpioset", value, 0);
	if (status < 0) {
		fprintf(stderr,
			"%s: error requesting GPIO line: %s\n",
			argv[0], gpiod_strerror(gpiod_errno()));
		return EXIT_FAILURE;
	}

	getchar();

	gpiod_chip_close(chip);

	return EXIT_SUCCESS;
}
