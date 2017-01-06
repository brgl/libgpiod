/*
 * List all GPIO chips.
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
	printf("Usage: %s <options>\n", get_progname());
	printf("List all GPIO chips\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
}

int main(int argc, char **argv)
{
	struct gpiod_chip_iter *iter;
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

	if (argc > 0)
		die("unrecognized argument: %s", argv[0]);

	iter = gpiod_chip_iter_new();
	if (!iter)
		die_perror("unable to access GPIO chips");

	gpiod_foreach_chip(iter, chip) {
		printf("%s [%s] (%u lines)\n",
		       gpiod_chip_name(chip),
		       gpiod_chip_label(chip),
		       gpiod_chip_num_lines(chip));
	}

	gpiod_chip_iter_free(iter);

	return EXIT_SUCCESS;
}
