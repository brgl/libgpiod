// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>
// SPDX-FileCopyrightText: 2022 Kent Gibson <warthog618@gmail.com>

#include <getopt.h>
#include <gpiod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tools-common.h"

struct config {
	bool active_low;
	bool by_name;
	bool numeric;
	bool strict;
	bool unquoted;
	enum gpiod_line_bias bias;
	enum gpiod_line_direction direction;
	unsigned int hold_period_us;
	const char *chip_id;
	const char *consumer;
};

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <line>...\n", get_prog_name());
	printf("\n");
	printf("Read values of GPIO lines.\n");
	printf("\n");
	printf("Lines are specified by name, or optionally by offset if the chip option\n");
	printf("is provided.\n");
	printf("\n");
	printf("Options:\n");
	printf("  -a, --as-is\t\tleave the line direction unchanged, not forced to input\n");
	print_bias_help();
	printf("      --by-name\t\ttreat lines as names even if they would parse as an offset\n");
	printf("  -c, --chip <chip>\trestrict scope to a particular chip\n");
	printf("  -C, --consumer <name>\tconsumer name applied to requested lines (default is 'gpioget')\n");
	printf("  -h, --help\t\tdisplay this help and exit\n");
	printf("  -l, --active-low\ttreat the line as active low\n");
	printf("  -p, --hold-period <period>\n");
	printf("\t\t\twait between requesting the lines and reading the values\n");
	printf("      --numeric\t\tdisplay line values as '0' (inactive) or '1' (active)\n");
	printf("  -s, --strict\t\tabort if requested line names are not unique\n");
	printf("      --unquoted\tdon't quote line names\n");
	printf("  -v, --version\t\toutput version information and exit\n");
	print_chip_help();
	print_period_help();
}

static int parse_config(int argc, char **argv, struct config *cfg)
{
	static const struct option longopts[] = {
		{ "active-low",	no_argument,		NULL,	'l' },
		{ "as-is",	no_argument,		NULL,	'a' },
		{ "bias",	required_argument,	NULL,	'b' },
		{ "by-name",	no_argument,		NULL,	'B' },
		{ "chip",	required_argument,	NULL,	'c' },
		{ "consumer",	required_argument,	NULL,	'C' },
		{ "help",	no_argument,		NULL,	'h' },
		{ "hold-period", required_argument,	NULL,	'p' },
		{ "numeric",	no_argument,		NULL,	'N' },
		{ "strict",	no_argument,		NULL,	's' },
		{ "unquoted",	no_argument,		NULL,	'Q' },
		{ "version",	no_argument,		NULL,	'v' },
		{ GETOPT_NULL_LONGOPT },
	};

	static const char *const shortopts = "+ab:c:C:hlp:sv";

	int opti, optc;

	memset(cfg, 0, sizeof(*cfg));
	cfg->direction = GPIOD_LINE_DIRECTION_INPUT;
	cfg->consumer = "gpioget";

	for (;;) {
		optc = getopt_long(argc, argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case 'a':
			cfg->direction = GPIOD_LINE_DIRECTION_AS_IS;
			break;
		case 'b':
			cfg->bias = parse_bias_or_die(optarg);
			break;
		case 'B':
			cfg->by_name = true;
			break;
		case 'c':
			cfg->chip_id = optarg;
			break;
		case 'C':
			cfg->consumer = optarg;
			break;
		case 'l':
			cfg->active_low = true;
			break;
		case 'N':
			cfg->numeric = true;
			break;
		case 'p':
			cfg->hold_period_us = parse_period_or_die(optarg);
			break;
		case 'Q':
			cfg->unquoted = true;
			break;
		case 's':
			cfg->strict = true;
			break;
		case 'h':
			print_help();
			exit(EXIT_SUCCESS);
		case 'v':
			print_version();
			exit(EXIT_SUCCESS);
		case '?':
			die("try %s --help", get_prog_name());
		case 0:
			break;
		default:
			abort();
		}
	}

	return optind;
}

int main(int argc, char **argv)
{
	struct gpiod_line_settings *settings;
	struct gpiod_request_config *req_cfg;
	struct gpiod_line_request *request;
	struct gpiod_line_config *line_cfg;
	struct line_resolver *resolver;
	enum gpiod_line_value *values;
	struct resolved_line *line;
	struct gpiod_chip *chip;
	unsigned int *offsets;
	int i, num_lines, ret;
	struct config cfg;
	const char *fmt;

	set_prog_name(argv[0]);
	i = parse_config(argc, argv, &cfg);
	argc -= i;
	argv += i;

	if (argc < 1)
		die("at least one GPIO line must be specified");

	resolver = resolve_lines(argc, argv, cfg.chip_id, cfg.strict,
				 cfg.by_name);
	validate_resolution(resolver, cfg.chip_id);

	offsets = calloc(resolver->num_lines, sizeof(*offsets));
	values = calloc(resolver->num_lines, sizeof(*values));
	if (!offsets || !values)
		die("out of memory");

	settings = gpiod_line_settings_new();
	if (!settings)
		die_perror("unable to allocate line settings");

	gpiod_line_settings_set_direction(settings, cfg.direction);

	if (cfg.bias)
		gpiod_line_settings_set_bias(settings, cfg.bias);

	if (cfg.active_low)
		gpiod_line_settings_set_active_low(settings, true);

	req_cfg = gpiod_request_config_new();
	if (!req_cfg)
		die_perror("unable to allocate the request config structure");

	line_cfg = gpiod_line_config_new();
	if (!line_cfg)
		die_perror("unable to allocate the line config structure");

	gpiod_request_config_set_consumer(req_cfg, cfg.consumer);

	for (i = 0; i < resolver->num_chips; i++) {
		chip = gpiod_chip_open(resolver->chips[i].path);
		if (!chip)
			die_perror("unable to open chip '%s'",
				   resolver->chips[i].path);

		num_lines = get_line_offsets_and_values(resolver, i, offsets,
							NULL);

		gpiod_line_config_reset(line_cfg);
		ret = gpiod_line_config_add_line_settings(line_cfg, offsets,
							  num_lines, settings);
		if (ret)
			die_perror("unable to add line settings");

		request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
		if (!request)
			die_perror("unable to request lines");

		if (cfg.hold_period_us)
			usleep(cfg.hold_period_us);

		ret = gpiod_line_request_get_values(request, values);
		if (ret)
			die_perror("unable to read GPIO line values");

		set_line_values(resolver, i, values);

		gpiod_line_request_release(request);
		gpiod_chip_close(chip);
	}

	fmt = cfg.unquoted ? "%s=%s" : "\"%s\"=%s";

	for (i = 0; i < resolver->num_lines; i++) {
		line = &resolver->lines[i];
		if (cfg.numeric)
			printf("%d", line->value);
		else
			printf(fmt, line->id,
			       line->value ? "active" : "inactive");

		if (i != resolver->num_lines - 1)
			printf(" ");
	}
	printf("\n");

	free_line_resolver(resolver);
	gpiod_request_config_free(req_cfg);
	gpiod_line_config_free(line_cfg);
	gpiod_line_settings_free(settings);
	free(offsets);
	free(values);

	return EXIT_SUCCESS;
}
