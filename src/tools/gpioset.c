/*
 * Set value of a GPIO line.
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
#include <sys/select.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/poll.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

static const struct option longopts[] = {
	{ "help",		no_argument,		NULL,	'h' },
	{ "version",		no_argument,		NULL,	'v' },
	{ "active-low",		no_argument,		NULL,	'l' },
	{ "mode",		required_argument,	NULL,	'm' },
	{ "sec",		required_argument,	NULL,	's' },
	{ "usec",		required_argument,	NULL,	'u' },
	{ 0 },
};

static const char *const shortopts = "+hvlm:s:u:";

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <chip name/number> <offset1>=<value1> <offset2>=<value2> ...\n",
	       get_progname());
	printf("Set GPIO line values of a GPIO chip\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
	printf("  -l, --active-low:\tset the line active state to low\n");
	printf("  -m, --mode=[exit|wait|time] (defaults to 'exit'):\n");
	printf("		tell the program what to do after setting values\n");
	printf("  -s, --sec=SEC:\tspecify the number of seconds to wait (only valid for --mode=time)\n");
	printf("  -u, --usec=USEC:\tspecify the number of microseconds to wait (only valid for --mode=time)\n");
	printf("\n");
	printf("Modes:\n");
	printf("  exit:\tset values and exit immediately\n");
	printf("  wait:\tset values and wait for user to press ENTER\n");
	printf("  time:\tset values and sleep for a specified amount of time\n");
	printf("  signal:\tset values and wait for SIGINT or SIGTERM\n");
}

struct callback_data {
	/* Replace with a union once we have more modes using callback data. */
	struct timeval tv;
};

static void wait_enter(void *data UNUSED)
{
	getchar();
}

static void wait_time(void *data)
{
	struct callback_data *cbdata = data;

	select(0, NULL, NULL, NULL, &cbdata->tv);
}

static void wait_signal(void *data UNUSED)
{
	int sigfd, status;
	struct pollfd pfd;
	sigset_t sigmask;

	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGTERM);
	sigaddset(&sigmask, SIGINT);

	status = sigprocmask(SIG_BLOCK, &sigmask, NULL);
	if (status < 0)
		die("sigprocmask: %s", strerror(errno));

	sigfd = signalfd(-1, &sigmask, 0);
	if (sigfd < 0)
		die("signalfd: %s", strerror(errno));

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = sigfd;
	pfd.events = POLLIN | POLLPRI;

	for (;;) {
		status = poll(&pfd, 1, 1000 /* one second */);
		if (status < 0)
			die("poll: %s", strerror(errno));
		else if (status > 0)
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
	gpiod_set_value_cb callback;
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

static const struct mode_mapping * parse_mode(const char *mode)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(modes); i++)
		if (strcmp(mode, modes[i].name) == 0)
			return &modes[i];

	return NULL;
}

int main(int argc, char **argv)
{
	const struct mode_mapping *mode = &modes[MODE_EXIT];
	int *values, status, optc, opti;
	bool active_low = false;
	unsigned int *offsets, num_lines, i;
	char *device, *end;
	struct callback_data cbdata;

	set_progname(argv[0]);

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
			active_low = true;
			break;
		case 'm':
			mode = parse_mode(optarg);
			if (!mode)
				die("invalid mode: %s", optarg);
			break;
		case 's':
			if (mode->id != MODE_TIME)
				die("--mode=time must be selected to specify seconds");
			cbdata.tv.tv_sec = strtoul(optarg, &end, 10);
			if (*end != '\0')
				die("invalid time value in seconds: %s", optarg);
			break;
		case 'u':
			if (mode->id != MODE_TIME)
				die("--mode=time must be selected to specify microseconds");
			cbdata.tv.tv_usec = strtoul(optarg, &end, 10);
			if (*end != '\0')
				die("invalid time value in microseconds: %s",
				    optarg);
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
		die("at least one gpio line offset to value mapping must be specified");

	device = argv[0];

	num_lines = argc - 1;

	offsets = malloc(sizeof(*offsets) * num_lines);
	values = malloc(sizeof(*values) * num_lines);
	if (!values || !offsets)
		die("out of memory");

	for (i = 0; i < num_lines; i++) {
		status = sscanf(argv[i + 1], "%u=%d", &offsets[i], &values[i]);
		if (status != 2)
			die("invalid offset<->value mapping: %s", argv[i + 1]);

		if (values[i] != 0 && values[i] != 1)
			die("value must be 0 or 1: %s", argv[i + 1]);

		if (offsets[i] > INT_MAX)
			die("invalid offset: %s", argv[i + 1]);
	}

	status = gpiod_simple_set_value_multiple(device, offsets, values,
						 num_lines, active_low,
						 mode->callback, &cbdata);
	if (status < 0)
		die_perror("error setting the GPIO line values");

	free(offsets);
	free(values);

	return EXIT_SUCCESS;
}
