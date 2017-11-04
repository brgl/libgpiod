/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include <gpiod.h>
#include "tools-common.h"

#include <stdio.h>
#include <string.h>
#include <getopt.h>

static const struct option longopts[] = {
	{ "help",	no_argument,	NULL,	'h' },
	{ "version",	no_argument,	NULL,	'v' },
	{ GETOPT_NULL_LONGOPT },
};

static const char *const shortopts = "+hv";

static void print_help(void)
{
	printf("Usage: %s [OPTIONS]\n", get_progname());
	printf("List all GPIO chips, print their labels and number of GPIO lines.\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
}

int main(int argc, char **argv)
{
	struct gpiod_chip_iter *iter;
	struct gpiod_chip *chip;
	int optc, opti;

	for (;;) {
		optc = getopt_long(argc, argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'h':
			print_help();
			return EXIT_SUCCESS;
		case 'v':
			print_version();
			return EXIT_SUCCESS;
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
