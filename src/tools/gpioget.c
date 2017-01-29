/*
 * Read value from GPIO line.
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
#include <limits.h>

static const struct option longopts[] = {
	{ "help",	no_argument,	NULL,	'h' },
	{ "version",	no_argument,	NULL,	'v' },
	{ "active-low",	no_argument,	NULL,	'l' },
	{ 0 },
};

static const char *const shortopts = "+hvl";

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <chip name/number> <offset 1> <offset 2> ...\n",
	       get_progname());
	printf("Read line value(s) from a GPIO chip\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
	printf("  -l, --active-low:\tset the line active state to low\n");
}

int main(int argc, char **argv)
{
	unsigned int *offsets, i, num_lines;
	int *values, optc, opti, status;
	bool active_low = false;
	char *device, *end;

	set_progname(argv[0]);

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
		case 'l':
			active_low = true;
			break;
		case '?':
			die("try %s --help", get_progname());
		default:
			abort();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1)
		die("gpiochip must be specified");

	if (argc < 2)
		die("at least one gpio line offset must be specified");

	device = argv[0];
	num_lines = argc - 1;

	values = malloc(sizeof(*values) * num_lines);
	offsets = malloc(sizeof(*offsets) * num_lines);
	if (!values || !offsets)
		die("out of memory");

	for (i = 0; i < num_lines; i++) {
		offsets[i] = strtoul(argv[i + 1], &end, 10);
		if (*end != '\0' || offsets[i] > INT_MAX)
			die("invalid GPIO offset: %s", argv[i + 1]);
	}

	status = gpiod_simple_get_value_multiple("gpioset", device,
						 offsets, values,
						 num_lines, active_low);
	if (status < 0)
		die_perror("error reading GPIO values");

	for (i = 0; i < num_lines; i++) {
		printf("%d", values[i]);
		if (i != num_lines - 1)
			printf(" ");
	}
	printf("\n");

	free(values);
	free(offsets);

	return EXIT_SUCCESS;
}
