/*
 * Monitor events on a GPIO line.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 */

#include <gpiod.h>
#include "tools-common.h"

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <limits.h>

static const struct option longopts[] = {
	{ "help",		no_argument,		NULL,	'h' },
	{ "version",		no_argument,		NULL,	'v' },
	{ "active-low",		no_argument,		NULL,	'l' },
	{ "num-events",		required_argument,	NULL,	'n' },
	{ "silent",		no_argument,		NULL,	's' },
	{ "rising-edge",	no_argument,		NULL,	'r' },
	{ "falling-edge",	no_argument,		NULL,	'f' },
	{ "format",		required_argument,	NULL,	'F' },
	{ 0 },
};

static const char *const shortopts = "+hvln:srfF:";

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <chip name/number> <line offset>\n",
	       get_progname());
	printf("Wait for events on a GPIO line\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
	printf("  -l, --active-low:\tset the line active state to low\n");
	printf("  -n, --num-events=NUM:\texit after processing NUM events\n");
	printf("  -s, --silent:\t\tdon't print event info\n");
	printf("  -r, --rising-edge:\tonly process rising edge events\n");
	printf("  -f, --falling-edge:\tonly process falling edge events\n");
	printf("  -F, --format=FMT\tspecify custom output format\n");
	printf("\n");
	printf("Format specifiers:\n");
	printf("  %%o:  GPIO line offset\n");
	printf("  %%e:  event type (0 - falling edge, 1 rising edge)\n");
	printf("  %%s:  seconds part of the event timestamp\n");
	printf("  %%n:  nanoseconds part of the event timestamp\n");
}

static volatile bool do_run = true;

static void sighandler(int signum UNUSED)
{
	do_run = false;
}

struct callback_data {
	unsigned int offset;
	unsigned int num_events_wanted;
	unsigned int num_events_done;
	bool silent;
	bool watch_rising;
	bool watch_falling;
	char *fmt;
};

static void event_print_custom(int type, const struct timespec *ts,
			       struct callback_data *data)
{
	char *prev, *curr, fmt;

	for (prev = curr = data->fmt;;) {
		curr = strchr(curr, '%');
		if (!curr) {
			fputs(prev, stdout);
			break;
		}

		if (prev != curr)
			fwrite(prev, curr - prev, 1, stdout);

		fmt = *(curr + 1);

		switch (fmt) {
		case 'o':
			printf("%u", data->offset);
			break;
		case 'e':
			if (type == GPIOD_EVENT_CB_RISING_EDGE)
				fputc('1', stdout);
			else
				fputc('0', stdout);
			break;
		case 's':
			printf("%ld", ts->tv_sec);
			break;
		case 'n':
			printf("%ld", ts->tv_nsec);
			break;
		case '%':
			fputc('%', stdout);
			break;
		case '\0':
			fputc('%', stdout);
			goto end;
		default:
			fwrite(curr, 2, 1, stdout);
			break;
		}

		curr += 2;
		prev = curr;
	}

end:
	fputc('\n', stdout);
}

static void event_print_human_readable(int type, const struct timespec *ts,
				       struct callback_data *data)
{
	char *evname;

	if (type == GPIOD_EVENT_CB_RISING_EDGE)
		evname = " RISING EDGE";
	else
		evname = "FALLING EDGE";

	printf("event: %s offset: %u timestamp: [%8ld.%09ld]\n",
	       evname, data->offset, ts->tv_sec, ts->tv_nsec);
}

static int event_callback(int type, const struct timespec *ts, void *data)
{
	struct callback_data *cbdata = data;

	if ((type == GPIOD_EVENT_CB_FALLING_EDGE && cbdata->watch_falling)
	    || (type == GPIOD_EVENT_CB_RISING_EDGE && cbdata->watch_rising)) {
		if (!cbdata->silent) {
			if (cbdata->fmt)
				event_print_custom(type, ts, cbdata);
			else
				event_print_human_readable(type, ts, cbdata);
		}
		cbdata->num_events_done++;
	}

	if (cbdata->num_events_wanted &&
	    cbdata->num_events_done >= cbdata->num_events_wanted)
		do_run = false;

	if (!do_run)
		return GPIOD_EVENT_CB_STOP;

	return GPIOD_EVENT_CB_OK;
}

int main(int argc, char **argv)
{
	struct callback_data cbdata;
	bool active_low = false;
	struct timespec timeout;
	int optc, opti, status;
	unsigned int offset;
	char *device, *end;

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
		case 'n':
			cbdata.num_events_wanted = strtoul(optarg, &end, 10);
			if (*end != '\0')
				die("invalid number: %s", optarg);
			break;
		case 's':
			cbdata.silent = true;
			break;
		case 'r':
			cbdata.watch_rising = true;
			break;
		case 'f':
			cbdata.watch_falling = true;
			break;
		case 'F':
			cbdata.fmt = optarg;
			break;
		case '?':
			die("try %s --help", get_progname());
		default:
			abort();
		}
	}

	if (!cbdata.watch_rising && !cbdata.watch_falling)
		cbdata.watch_rising = cbdata.watch_falling = true;

	argc -= optind;
	argv += optind;

	if (argc < 1)
		die("gpiochip must be specified");

	if (argc < 2)
		die("GPIO line offset must be specified");

	if (argc > 2)
		die("watching more than one GPIO line unsupported");

	device = argv[0];
	offset = strtoul(argv[1], &end, 10);
	if (*end != '\0' || offset > INT_MAX)
		die("invalid GPIO offset: %s", argv[1]);

	cbdata.offset = offset;

	timeout.tv_sec = 0;
	timeout.tv_nsec = 500000000;

	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	status = gpiod_simple_event_loop("gpiomon", device, offset, active_low,
					 &timeout, event_callback, &cbdata);
	if (status < 0)
		die_perror("error waiting for events");

	return EXIT_SUCCESS;
}
