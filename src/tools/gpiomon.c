/*
 * Monitor events on a GPIO line.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */

#include <gpiod.h>
#include "tools-common.h"

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>

static const struct option longopts[] = {
	{ "help",	no_argument,	NULL,	'h' },
	{ "version",	no_argument,	NULL,	'v' },
	{ "active-low",	no_argument,	NULL,	'l' },
	{ 0 },
};

static const char *const shortopts = "hvl";

static void print_help(void)
{
	printf("Usage: %s [CHIP NAME/NUMBER] [LINE OFFSET] <options>\n",
	       get_progname());
	printf("Wait for events on a GPIO line\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
	printf("  -l, --active-low:\tset the line active state to low\n");
}

static volatile bool do_run = true;

static void sighandler(int signum UNUSED)
{
	do_run = false;
}

static int event_callback(int type, const struct timespec *ts,
			  void *data UNUSED)
{
	const char *evname = NULL;

	switch (type) {
	case GPIOD_EVENT_CB_RISING_EDGE:
		evname = " RISING EDGE";
		break;
	case GPIOD_EVENT_CB_FALLING_EDGE:
		evname = "FALLING_EDGE";
		break;
	default:
		break;
	}

	if (evname)
		printf("GPIO EVENT: %s [%8ld.%09ld]\n",
		       evname, ts->tv_sec, ts->tv_nsec);

	if (!do_run)
		return GPIOD_EVENT_CB_STOP;

	return GPIOD_EVENT_CB_OK;
}

int main(int argc, char **argv)
{
	bool active_low = false;
	struct timespec timeout;
	int optc, opti, status;
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

	device = argv[0];
	offset = strtoul(argv[1], &end, 10);
	if (*end != '\0')
		die("invalid GPIO offset: %s", argv[1]);

	timeout.tv_sec = 0;
	timeout.tv_nsec = 500000000;

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	status = gpiod_simple_event_loop(device, offset, active_low,
					 &timeout, event_callback, NULL);
	if (status < 0)
		die_perror("error waiting for events");

	return EXIT_SUCCESS;
}
