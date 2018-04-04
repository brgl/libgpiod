// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <gpiod.h>
#include "tools-common.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <getopt.h>
#include <errno.h>

typedef bool (*is_set_func)(struct gpiod_line *);

struct flag {
	const char *name;
	is_set_func is_set;
};

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
	printf("Print information about all lines of the specified GPIO chip(s) (or all gpiochips if none are specified).\n");
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
	int status;

	va_start(va, fmt);
	status = vasprintf(&buf, fmt, va);
	va_end(va);
	if (status < 0)
		die("vasprintf: %s\n", strerror(errno));

	len = strlen(buf) - 1;

	if (len >= prlen || *of) {
		*of = true;
		printf("%s", buf);
	} else {
		status = asprintf(&buffmt, "%%%us", prlen);
		if (status < 0)
			die("asprintf: %s\n", strerror(errno));

		printf(buffmt, buf);
	}

	free(buf);
	if (fmt)
		free(buffmt);
}

static void list_lines(struct gpiod_chip *chip)
{
	struct gpiod_line_iter *iter;
	int direction, active_state;
	const char *name, *consumer;
	struct gpiod_line *line;
	unsigned int i, offset;
	bool flag_printed, of;

	iter = gpiod_line_iter_new(chip);
	if (!iter)
		die_perror("error creating line iterator");

	printf("%s - %u lines:\n",
	       gpiod_chip_name(chip), gpiod_chip_num_lines(chip));

	gpiod_foreach_line(iter, line) {
		offset = gpiod_line_offset(line);
		name = gpiod_line_name(line);
		consumer = gpiod_line_consumer(line);
		direction = gpiod_line_direction(line);
		active_state = gpiod_line_active_state(line);

		of = false;

		printf("\tline ");
		prinfo(&of, 3, "%u", offset);
		printf(": ");

		name ? prinfo(&of, 12, "\"%s\"", name)
		     : prinfo(&of, 12, "unnamed");
		printf(" ");

		consumer ? prinfo(&of, 12, "\"%s\"", consumer)
			 : prinfo(&of, 12, "unused");
		printf(" ");

		prinfo(&of, 8, "%s ", direction == GPIOD_LINE_DIRECTION_INPUT
							? "input" : "output");
		prinfo(&of, 13, "%s ",
		       active_state == GPIOD_LINE_ACTIVE_STATE_LOW
							? "active-low"
							: "active-high");

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

	gpiod_line_iter_free(iter);
}

int main(int argc, char **argv)
{
	struct gpiod_chip_iter *chip_iter;
	struct gpiod_chip *chip;
	int i, optc, opti;

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
		chip_iter = gpiod_chip_iter_new();
		if (!chip_iter)
			die_perror("error accessing GPIO chips");

		gpiod_foreach_chip(chip_iter, chip)
			list_lines(chip);

		gpiod_chip_iter_free(chip_iter);
	} else {
		for (i = 0; i < argc; i++) {
			chip = gpiod_chip_open_lookup(argv[i]);
			if (!chip)
				die_perror("looking up chip %s", argv[i]);

			list_lines(chip);

			gpiod_chip_close(chip);
		}
	}

	return EXIT_SUCCESS;
}
