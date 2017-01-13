/*
 * Set value of a GPIO line.
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
	{ "version",	no_argument,	NULL,	'v' },
	{ "active-low",	no_argument,	NULL,	'l' },
	{ 0 },
};

static const char *const shortopts = "hvl";

static void print_help(void)
{
	printf("Usage: %s [CHIP NAME/NUMBER] [LINE OFFSET] [VALUE] <options>\n",
	       get_progname());
	printf("Set value of a GPIO line\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
	printf("  -l, --active-low:\tset the line active state to low\n");
	printf("\n");
	printf("This program reserves the GPIO line, sets its value and waits for the user to press ENTER before releasing the line\n");
}

static void wait_for_enter(void *data UNUSED)
{
	getchar();
}

int main(int argc, char **argv)
{
	int value, status, optc, opti;
	bool active_low = false;
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
		die("gpio line offset must be specified");

	if (argc < 3)
		die("value must be specified");

	device = argv[0];

	offset = strtoul(argv[1], &end, 10);
	if (*end != '\0')
		die("invalid GPIO offset: %s", argv[1]);

	value = strtoul(argv[2], &end, 10);
	if (*end != '\0' || (value != 0 && value != 1))
		die("invalid value: %s", argv[2]);

	status = gpiod_simple_set_value(device, offset, value,
					active_low, wait_for_enter, NULL);
	if (status < 0)
		die_perror("error setting the GPIO line value");

	return EXIT_SUCCESS;
}
