/*
 * Read value from GPIO line.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */

#include "gpiod.h"
#include "tools-common.h"

#include <stdio.h>
#include <string.h>
#include <getopt.h>

static const struct option longopts[] = {
	{ "help", no_argument, NULL, 'h' },
	{ 0 },
};

static const char *const shortopts = "+h";

static void print_help(void)
{
	printf("Usage: %s <options> [NAME]\n", get_progname());
	printf("Find a GPIO line by name.\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
}

int main(int argc, char **argv)
{
	struct gpiod_line *line;
	struct gpiod_chip *chip;
	int optc, opti;

	set_progname(argv[0]);

	for (;;) {
		optc = getopt_long(argc, argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'h':
			print_help();
			exit(EXIT_SUCCESS);
		case '?':
			die("try %s --help", get_progname());
		default:
			abort();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		die("GPIO line name must be specified\n");

	line = gpiod_line_find_by_name(argv[1]);
	if (!line)
		return EXIT_FAILURE;

	chip = gpiod_line_get_chip(line);

	printf("%s %u\n", gpiod_chip_name(chip), gpiod_line_offset(line));

	gpiod_chip_close(chip);

	return EXIT_SUCCESS;
}
