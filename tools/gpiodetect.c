// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <gpiod.h>
#include <stdio.h>
#include <string.h>

#include "tools-common.h"

static const struct option longopts[] = {
	{ "help",	no_argument,	NULL,	'h' },
	{ "version",	no_argument,	NULL,	'v' },
	{ GETOPT_NULL_LONGOPT },
};

static const char *const shortopts = "+hv";

static void print_help(void)
{
	printf("Usage: %s [OPTIONS]\n", get_progname());
	printf("\n");
	printf("List all GPIO chips, print their labels and number of GPIO lines.\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
}

int main(int argc, char **argv)
{
	int optc, opti, num_chips, i;
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

	if (argc > 0)
		die("unrecognized argument: %s", argv[0]);

	num_chips = scandir("/dev/", &entries, chip_dir_filter, alphasort);
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

		printf("%s [%s] (%u lines)\n",
		       gpiod_chip_name(chip),
		       gpiod_chip_label(chip),
		       gpiod_chip_num_lines(chip));

		gpiod_chip_unref(chip);
		free(entries[i]);
	}

	free(entries);

	return EXIT_SUCCESS;
}
