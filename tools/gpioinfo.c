// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <gpiod.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "tools-common.h"

typedef bool (*is_set_func)(struct gpiod_line *);

struct flag {
	const char *name;
	is_set_func is_set;
};

static bool line_bias_is_pullup(struct gpiod_line *line)
{
	return gpiod_line_bias(line) == GPIOD_LINE_BIAS_PULL_UP;
}

static bool line_bias_is_pulldown(struct gpiod_line *line)
{
	return gpiod_line_bias(line) == GPIOD_LINE_BIAS_PULL_DOWN;
}

static bool line_bias_is_disabled(struct gpiod_line *line)
{
	return gpiod_line_bias(line) == GPIOD_LINE_BIAS_DISABLED;
}

static const struct flag flags[] = {
	{
		.name = "used",
		.is_set = gpiod_line_is_used,
	},
	{
		.name = "open-drain",
		.is_set = gpiod_line_is_open_drain,
	},
	{
		.name = "open-source",
		.is_set = gpiod_line_is_open_source,
	},
	{
		.name = "pull-up",
		.is_set = line_bias_is_pullup,
	},
	{
		.name = "pull-down",
		.is_set = line_bias_is_pulldown,
	},
	{
		.name = "bias-disabled",
		.is_set = line_bias_is_disabled,
	},
};

static const struct option longopts[] = {
	{ "help",	no_argument,	NULL,	'h' },
	{ "version",	no_argument,	NULL,	'v' },
	{ GETOPT_NULL_LONGOPT },
};

static const char *const shortopts = "+hv";

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <gpiochip1> ...\n", get_progname());
	printf("\n");
	printf("Print information about all lines of the specified GPIO chip(s) (or all gpiochips if none are specified).\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
}

static PRINTF(3, 4) void prinfo(bool *of,
				unsigned int prlen, const char *fmt, ...)
{
	char *buf, *buffmt = NULL;
	size_t len;
	va_list va;
	int rv;

	va_start(va, fmt);
	rv = vasprintf(&buf, fmt, va);
	va_end(va);
	if (rv < 0)
		die("vasprintf: %s\n", strerror(errno));

	len = strlen(buf) - 1;

	if (len >= prlen || *of) {
		*of = true;
		printf("%s", buf);
	} else {
		rv = asprintf(&buffmt, "%%%us", prlen);
		if (rv < 0)
			die("asprintf: %s\n", strerror(errno));

		printf(buffmt, buf);
	}

	free(buf);
	if (fmt)
		free(buffmt);
}

static void list_lines(struct gpiod_chip *chip)
{
	bool flag_printed, of, active_low;
	const char *name, *consumer;
	struct gpiod_line *line;
	unsigned int i, offset;
	int direction;

	printf("%s - %u lines:\n",
	       gpiod_chip_name(chip), gpiod_chip_num_lines(chip));

	for (offset = 0; offset < gpiod_chip_num_lines(chip); offset++) {
		line = gpiod_chip_get_line(chip, offset);
		if (!line)
			die_perror("unable to retrieve the line object from chip");

		name = gpiod_line_name(line);
		consumer = gpiod_line_consumer(line);
		direction = gpiod_line_direction(line);
		active_low = gpiod_line_is_active_low(line);

		of = false;

		printf("\tline ");
		prinfo(&of, 3, "%u", offset);
		printf(": ");

		name ? prinfo(&of, 12, "\"%s\"", name)
		     : prinfo(&of, 12, "unnamed");
		printf(" ");

		if (!gpiod_line_is_used(line))
			prinfo(&of, 12, "unused");
		else
			consumer ? prinfo(&of, 12, "\"%s\"", consumer)
				 : prinfo(&of, 12, "kernel");

		printf(" ");

		prinfo(&of, 8, "%s ", direction == GPIOD_LINE_DIRECTION_INPUT
							? "input" : "output");
		prinfo(&of, 13, "%s ",
		       active_low ? "active-low" : "active-high");

		flag_printed = false;
		for (i = 0; i < ARRAY_SIZE(flags); i++) {
			if (flags[i].is_set(line)) {
				if (flag_printed)
					printf(" ");
				else
					printf("[");
				printf("%s", flags[i].name);
				flag_printed = true;
			}
		}
		if (flag_printed)
			printf("]");

		printf("\n");
	}
}

int main(int argc, char **argv)
{
	int num_chips, i, optc, opti;
	struct gpiod_chip *chip;
	struct dirent **entries;

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

	if (argc == 0) {
		num_chips = scandir("/dev/", &entries,
				    chip_dir_filter, alphasort);
		if (num_chips < 0)
			die_perror("unable to scan /dev");

		for (i = 0; i < num_chips; i++) {
			chip = chip_open_by_name(entries[i]->d_name);
			if (!chip) {
				if (errno == EACCES)
					printf("%s Permission denied\n",
					       entries[i]->d_name);
				else
					die_perror("unable to open %s",
						   entries[i]->d_name);
			}

			list_lines(chip);

			gpiod_chip_close(chip);
		}
	} else {
		for (i = 0; i < argc; i++) {
			chip = chip_open_lookup(argv[i]);
			if (!chip)
				die_perror("looking up chip %s", argv[i]);

			list_lines(chip);

			gpiod_chip_close(chip);
		}
	}

	return EXIT_SUCCESS;
}
