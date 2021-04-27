// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <getopt.h>
#include <gpiod.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "tools-common.h"

static const struct option longopts[] = {
	{ "help",	no_argument,		NULL,	'h' },
	{ "version",	no_argument,		NULL,	'v' },
	{ "active-low",	no_argument,		NULL,	'l' },
	{ "dir-as-is",	no_argument,		NULL,	'n' },
	{ "bias",	required_argument,	NULL,	'B' },
	{ GETOPT_NULL_LONGOPT },
};

static const char *const shortopts = "+hvlnB:";

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <chip name/number> <offset 1> <offset 2> ...\n",
	       get_progname());
	printf("\n");
	printf("Read line value(s) from a GPIO chip\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
	printf("  -l, --active-low:\tset the line active state to low\n");
	printf("  -n, --dir-as-is:\tdon't force-reconfigure line direction\n");
	printf("  -B, --bias=[as-is|disable|pull-down|pull-up] (defaults to 'as-is'):\n");
	printf("		set the line bias\n");
	printf("\n");
	print_bias_help();
}

int main(int argc, char **argv)
{
	int request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
	struct gpiod_line_request_config config;
	int *values, optc, opti, rv, flags = 0;
	unsigned int *offsets, i, num_lines;
	struct gpiod_line_bulk *lines;
	struct gpiod_chip *chip;
	char *device, *end;

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
			flags |= GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;
			break;
		case 'n':
			request_type = GPIOD_LINE_REQUEST_DIRECTION_AS_IS;
			break;
		case 'B':
			flags |= bias_flags(optarg);
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
		die("at least one GPIO line offset must be specified");

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

	chip = chip_open_lookup(device);
	if (!chip)
		die_perror("unable to open %s", device);

	lines = gpiod_chip_get_lines(chip, offsets, num_lines);
	if (!lines)
		die_perror("unable to retrieve GPIO lines from chip");

	memset(&config, 0, sizeof(config));

	config.consumer = "gpioget";
	config.request_type = request_type;
	config.flags = flags;

	rv = gpiod_line_request_bulk(lines, &config, NULL);
	if (rv)
		die_perror("unable to request lines");

	rv = gpiod_line_get_value_bulk(lines, values);
	if (rv < 0)
		die_perror("error reading GPIO values");

	for (i = 0; i < num_lines; i++) {
		printf("%d", values[i]);
		if (i != num_lines - 1)
			printf(" ");
	}
	printf("\n");

	gpiod_line_release_bulk(lines);
	gpiod_chip_unref(chip);
	gpiod_line_bulk_free(lines);
	free(values);
	free(offsets);

	return EXIT_SUCCESS;
}
