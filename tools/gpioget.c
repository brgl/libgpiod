// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <getopt.h>
#include <gpiod.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
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
	int direction = GPIOD_LINE_DIRECTION_INPUT;
	int optc, opti, bias = 0, ret, *values;
	struct gpiod_line_settings *settings;
	struct gpiod_request_config *req_cfg;
	struct gpiod_line_request *request;
	struct gpiod_line_config *line_cfg;
	struct gpiod_chip *chip;
	bool active_low = false;
	unsigned int *offsets;
	size_t i, num_lines;
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
			active_low = true;
			break;
		case 'n':
			direction = GPIOD_LINE_DIRECTION_AS_IS;
			break;
		case 'B':
			bias = parse_bias(optarg);
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

	offsets = calloc(num_lines, sizeof(*offsets));
	values = calloc(num_lines, sizeof(*values));
	if (!offsets || ! values)
		die("out of memory");

	for (i = 0; i < num_lines; i++) {
		offsets[i] = strtoul(argv[i + 1], &end, 10);
		if (*end != '\0' || offsets[i] > INT_MAX)
			die("invalid GPIO offset: %s", argv[i + 1]);
	}

	if (has_duplicate_offsets(num_lines, offsets))
		die("offsets must be unique");

	chip = chip_open_lookup(device);
	if (!chip)
		die_perror("unable to open %s", device);

	settings = gpiod_line_settings_new();
	if (!settings)
		die_perror("unable to allocate line settings");

	gpiod_line_settings_set_direction(settings, direction);

	if (bias)
		gpiod_line_settings_set_bias(settings, bias);

	if (active_low)
		gpiod_line_settings_set_active_low(settings, active_low);

	req_cfg = gpiod_request_config_new();
	if (!req_cfg)
		die_perror("unable to allocate the request config structure");

	gpiod_request_config_set_consumer(req_cfg, "gpioget");

	line_cfg = gpiod_line_config_new();
	if (!line_cfg)
		die_perror("unable to allocate the line config structure");

	ret = gpiod_line_config_add_line_settings(line_cfg, offsets,
						  num_lines, settings);
	if (ret)
		die_perror("unable to add line settings");

	request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
	if (!request)
		die_perror("unable to request lines");

	ret = gpiod_line_request_get_values(request, values);
	if (ret)
		die_perror("unable to read GPIO line values");

	for (i = 0; i < num_lines; i++) {
		printf("%d", values[i]);
		if (i != num_lines - 1)
			printf(" ");
	}
	printf("\n");

	gpiod_line_request_release(request);
	gpiod_request_config_free(req_cfg);
	gpiod_line_config_free(line_cfg);
	gpiod_line_settings_free(settings);
	gpiod_chip_close(chip);
	free(offsets);
	free(values);

	return EXIT_SUCCESS;
}
