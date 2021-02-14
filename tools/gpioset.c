// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <errno.h>
#include <gpiod.h>
#include <getopt.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "tools-common.h"

static const struct option longopts[] = {
	{ "help",		no_argument,		NULL,	'h' },
	{ "version",		no_argument,		NULL,	'v' },
	{ "active-low",		no_argument,		NULL,	'l' },
	{ "bias",		required_argument,	NULL,	'B' },
	{ "drive",		required_argument,	NULL,	'D' },
	{ "mode",		required_argument,	NULL,	'm' },
	{ "sec",		required_argument,	NULL,	's' },
	{ "usec",		required_argument,	NULL,	'u' },
	{ "background",		no_argument,		NULL,	'b' },
	{ GETOPT_NULL_LONGOPT },
};

static const char *const shortopts = "+hvlB:D:m:s:u:b";

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <chip name/number> <offset1>=<value1> <offset2>=<value2> ...\n",
	       get_progname());
	printf("\n");
	printf("Set GPIO line values of a GPIO chip and maintain the state until the process exits\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
	printf("  -l, --active-low:\tset the line active state to low\n");
	printf("  -B, --bias=[as-is|disable|pull-down|pull-up] (defaults to 'as-is'):\n");
	printf("		set the line bias\n");
	printf("  -D, --drive=[push-pull|open-drain|open-source] (defaults to 'push-pull'):\n");
	printf("		set the line drive mode\n");
	printf("  -m, --mode=[exit|wait|time|signal] (defaults to 'exit'):\n");
	printf("		tell the program what to do after setting values\n");
	printf("  -s, --sec=SEC:\tspecify the number of seconds to wait (only valid for --mode=time)\n");
	printf("  -u, --usec=USEC:\tspecify the number of microseconds to wait (only valid for --mode=time)\n");
	printf("  -b, --background:\tafter setting values: detach from the controlling terminal\n");
	printf("\n");
	print_bias_help();
	printf("\n");
	printf("Drives:\n");
	printf("  push-pull:\tdrive the line both high and low\n");
	printf("  open-drain:\tdrive the line low or go high impedance\n");
	printf("  open-source:\tdrive the line high or go high impedance\n");
	printf("\n");
	printf("Modes:\n");
	printf("  exit:\t\tset values and exit immediately\n");
	printf("  wait:\t\tset values and wait for user to press ENTER\n");
	printf("  time:\t\tset values and sleep for a specified amount of time\n");
	printf("  signal:\tset values and wait for SIGINT or SIGTERM\n");
	printf("\n");
	printf("Note: the state of a GPIO line controlled over the character device reverts to default\n");
	printf("when the last process referencing the file descriptor representing the device file exits.\n");
	printf("This means that it's wrong to run gpioset, have it exit and expect the line to continue\n");
	printf("being driven high or low. It may happen if given pin is floating but it must be interpreted\n");
	printf("as undefined behavior.\n");
}

struct callback_data {
	/* Replace with a union once we have more modes using callback data. */
	struct timeval tv;
	bool daemonize;
};

static void maybe_daemonize(bool daemonize)
{
	int rv;

	if (daemonize) {
		rv = daemon(0, 0);
		if (rv < 0)
			die("unable to daemonize: %s", strerror(errno));
	}
}

static void wait_enter(void *data UNUSED)
{
	getchar();
}

static void wait_time(void *data)
{
	struct callback_data *cbdata = data;

	maybe_daemonize(cbdata->daemonize);
	select(0, NULL, NULL, NULL, &cbdata->tv);
}

static void wait_signal(void *data)
{
	struct callback_data *cbdata = data;
	struct pollfd pfd;
	int sigfd, rv;

	sigfd = make_signalfd();

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = sigfd;
	pfd.events = POLLIN | POLLPRI;

	maybe_daemonize(cbdata->daemonize);

	for (;;) {
		rv = poll(&pfd, 1, 1000 /* one second */);
		if (rv < 0)
			die("error polling for signals: %s", strerror(errno));
		else if (rv > 0)
			break;
	}

	/*
	 * Don't bother reading siginfo - it's enough to know that we
	 * received any signal.
	 */
	close(sigfd);
}

enum {
	MODE_EXIT = 0,
	MODE_WAIT,
	MODE_TIME,
	MODE_SIGNAL,
};

struct mode_mapping {
	int id;
	const char *name;
	void (*callback)(void *);
};

static const struct mode_mapping modes[] = {
	[MODE_EXIT] = {
		.id		= MODE_EXIT,
		.name		= "exit",
		.callback	= NULL,
	},
	[MODE_WAIT] = {
		.id		= MODE_WAIT,
		.name		= "wait",
		.callback	= wait_enter,
	},
	[MODE_TIME] = {
		.id		= MODE_TIME,
		.name		= "time",
		.callback	= wait_time,
	},
	[MODE_SIGNAL] = {
		.id		= MODE_SIGNAL,
		.name		= "signal",
		.callback	= wait_signal,
	},
};

static const struct mode_mapping *parse_mode(const char *mode)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(modes); i++)
		if (strcmp(mode, modes[i].name) == 0)
			return &modes[i];

	return NULL;
}

static int drive_flags(const char *option)
{
	if (strcmp(option, "open-drain") == 0)
		return GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN;
	if (strcmp(option, "open-source") == 0)
		return GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE;
	if (strcmp(option, "push-pull") != 0)
		die("invalid drive: %s", option);
	return 0;
}

int main(int argc, char **argv)
{
	const struct mode_mapping *mode = &modes[MODE_EXIT];
	struct gpiod_line_request_config config;
	int *values, rv, optc, opti, flags = 0;
	unsigned int *offsets, num_lines, i;
	struct gpiod_line_bulk *lines;
	struct callback_data cbdata;
	struct gpiod_chip *chip;
	char *device, *end;

	memset(&cbdata, 0, sizeof(cbdata));

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
		case 'B':
			flags |= bias_flags(optarg);
			break;
		case 'D':
			flags |= drive_flags(optarg);
			break;
		case 'm':
			mode = parse_mode(optarg);
			if (!mode)
				die("invalid mode: %s", optarg);
			break;
		case 's':
			cbdata.tv.tv_sec = strtoul(optarg, &end, 10);
			if (*end != '\0')
				die("invalid time value in seconds: %s", optarg);
			break;
		case 'u':
			cbdata.tv.tv_usec = strtoul(optarg, &end, 10);
			if (*end != '\0')
				die("invalid time value in microseconds: %s",
				    optarg);
			break;
		case 'b':
			cbdata.daemonize = true;
			break;
		case '?':
			die("try %s --help", get_progname());
		default:
			abort();
		}
	}

	argc -= optind;
	argv += optind;

	if (mode->id != MODE_TIME && (cbdata.tv.tv_sec || cbdata.tv.tv_usec))
		die("can't specify wait time in this mode");

	if (mode->id != MODE_SIGNAL &&
	    mode->id != MODE_TIME &&
	    cbdata.daemonize)
		die("can't daemonize in this mode");

	if (argc < 1)
		die("gpiochip must be specified");

	if (argc < 2)
		die("at least one GPIO line offset to value mapping must be specified");

	device = argv[0];

	num_lines = argc - 1;

	offsets = malloc(sizeof(*offsets) * num_lines);
	values = malloc(sizeof(*values) * num_lines);
	if (!values || !offsets)
		die("out of memory");

	for (i = 0; i < num_lines; i++) {
		rv = sscanf(argv[i + 1], "%u=%d", &offsets[i], &values[i]);
		if (rv != 2)
			die("invalid offset<->value mapping: %s", argv[i + 1]);

		if (values[i] != 0 && values[i] != 1)
			die("value must be 0 or 1: %s", argv[i + 1]);

		if (offsets[i] > INT_MAX)
			die("invalid offset: %s", argv[i + 1]);
	}

	chip = chip_open_lookup(device);
	if (!chip)
		die_perror("unable to open %s", device);

	lines = gpiod_chip_get_lines(chip, offsets, num_lines);
	if (!lines)
		die_perror("unable to retrieve GPIO lines from chip");

	memset(&config, 0, sizeof(config));

	config.consumer = "gpioset";
	config.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
	config.flags = flags;

	rv = gpiod_line_request_bulk(lines, &config, values);
	if (rv)
		die_perror("unable to request lines");

	mode->callback(&cbdata);

	gpiod_line_release_bulk(lines);
	gpiod_chip_close(chip);
	gpiod_line_bulk_free(lines);
	free(offsets);
	free(values);

	return EXIT_SUCCESS;
}
