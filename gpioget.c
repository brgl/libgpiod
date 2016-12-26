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
	unsigned int offset;
	char *device, *end;
	int value;

	if (argc < 2) {
		fprintf(stderr, "%s: gpiochip must be specified\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (argc < 3) {
		fprintf(stderr,
			"%s: gpio line offset must be specified\n", argv[0]);
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

	value = gpiod_simple_get_value(device, offset);
	if (value < 0) {
		fprintf(stderr,
			"%s: error reading GPIO value: %s\n",
			argv[0], strerror(-value));
		return EXIT_FAILURE;
	}

	printf("%d\n", value);

	return EXIT_SUCCESS;
}
