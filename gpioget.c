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
	{ "help",	no_argument,	NULL,	'h' },
	{ "active-low",	no_argument,	NULL,	'l' },
	{ 0 },
};

static const char *const shortopts = "hl";

static void print_help(void)
{
	printf("Usage: %s [CHIP NAME/NUMBER] [LINE OFFSET] <options>\n",
	       get_progname());
	printf("List all GPIO chips\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -l, --active-low:\tset the line active state to low\n");
}

int main(int argc, char **argv)
{
	bool active_low = false;
	int value, optc, opti;
	unsigned int offset;
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
		die("gpio line offset must be specified");

	device = argv[0];
	offset = strtoul(argv[1], &end, 10);
	if (*end != '\0')
		die("invalid GPIO offset: %s", argv[1]);

	value = gpiod_simple_get_value(device, offset, active_low);
	if (value < 0)
		die_perror("error reading GPIO value");

	printf("%d\n", value);

	return EXIT_SUCCESS;
}
