/*
 * Monitor events on a GPIO line.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */

#include "gpiod.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
	struct gpiod_line_evreq_config config;
	struct gpiod_line_event event;
	struct timespec timeout;
	struct gpiod_chip *chip;
	struct gpiod_line *line;
	unsigned int offset;
	char *device, *end;
	int status;

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

	memset(&config, 0, sizeof(config));
	config.consumer = "gpiomon";
	config.event_type = GPIOD_EVENT_BOTH_EDGES;

	status = gpiod_line_event_request(line, &config);
	if (status < 0) {
		fprintf(stderr,
			"%s: error requesting line event: %s\n",
			argv[0], gpiod_strerror(gpiod_errno()));
		return EXIT_FAILURE;
	}

	for (;;) {
		timeout.tv_sec = 1;
		timeout.tv_nsec = 0;

		status = gpiod_line_event_wait(line, &timeout);
		if (status < 0) {
			fprintf(stderr,
				"%s: error waiting for line event: %s\n",
				argv[0], gpiod_strerror(gpiod_errno()));
			return EXIT_FAILURE;
		} else if (status == 0) {
			continue;
		}

		status = gpiod_line_event_read(line, &event);
		if (status < 0) {
			fprintf(stderr,
				"%s: error reading the line event: %s\n",
				argv[0], gpiod_strerror(gpiod_errno()));
			return EXIT_FAILURE;
		}

		printf("GPIO EVENT: %s [%ld.%ld]\n",
		       event.event_type == GPIOD_EVENT_FALLING_EDGE
							? "FALLING EDGE"
							: "RAISING EDGE",
		       event.ts.tv_sec, event.ts.tv_nsec);
	}

	gpiod_line_event_release(line);
	gpiod_line_release(line);
	gpiod_chip_close(chip);

	return EXIT_SUCCESS;
}
