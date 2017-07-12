/*
 * This file is part of libgpiod.
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
#include <sys/signalfd.h>
#include <limits.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>

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
	printf("Usage: %s [OPTIONS] <chip name/number> <offset 1> <offset 2> ...\n",
	       get_progname());
	printf("Wait for events on GPIO lines\n");
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

struct mon_ctx {
	unsigned int offset;
	unsigned int events_wanted;
	unsigned int events_done;
	bool silent;
	char *fmt;
	bool stop;
};

static void event_print_custom(unsigned int offset,
			       struct gpiod_line_event *ev,
			       struct mon_ctx *ctx)
{
	char *prev, *curr, fmt;

	for (prev = curr = ctx->fmt;;) {
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
			printf("%u", offset);
			break;
		case 'e':
			if (ev->event_type == GPIOD_EVENT_RISING_EDGE)
				fputc('1', stdout);
			else
				fputc('0', stdout);
			break;
		case 's':
			printf("%ld", ev->ts.tv_sec);
			break;
		case 'n':
			printf("%ld", ev->ts.tv_nsec);
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

static void event_print_human_readable(unsigned int offset,
				       struct gpiod_line_event *ev)
{
	char *evname;

	if (ev->event_type == GPIOD_EVENT_RISING_EDGE)
		evname = " RISING EDGE";
	else
		evname = "FALLING EDGE";

	printf("event: %s offset: %u timestamp: [%8ld.%09ld]\n",
	       evname, offset, ev->ts.tv_sec, ev->ts.tv_nsec);
}

static void handle_event(unsigned int offset,
			 struct gpiod_line_event *ev, struct mon_ctx *ctx)
{
	if (!ctx->silent) {
		if (ctx->fmt)
			event_print_custom(offset, ev, ctx);
		else
			event_print_human_readable(offset, ev);
	}
	ctx->events_done++;

	if (ctx->events_wanted && ctx->events_done >= ctx->events_wanted)
		ctx->stop = true;
}

static int make_signalfd(void)
{
	sigset_t sigmask;
	int sigfd, rv;

	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGTERM);
	sigaddset(&sigmask, SIGINT);

	rv = sigprocmask(SIG_BLOCK, &sigmask, NULL);
	if (rv < 0)
		die("error masking signals: %s", strerror(errno));

	sigfd = signalfd(-1, &sigmask, 0);
	if (sigfd < 0)
		die("error creating signalfd: %s", strerror(errno));

	return sigfd;
}

int main(int argc, char **argv)
{
	bool watch_rising = false, watch_falling = false, active_low = false;
	struct gpiod_line_bulk linebulk = GPIOD_LINE_BULK_INITIALIZER;
	int optc, opti, i, rv, sigfd, num_lines = 0, evdone, numev;
	struct gpiod_line_request_config evconf;
	struct gpiod_line_event evbuf;
	struct gpiod_line *line;
	struct gpiod_chip *chip;
	struct mon_ctx config;
	struct pollfd *pfds;
	unsigned int offset;
	char *end;

	set_progname(argv[0]);

	memset(&config, 0, sizeof(config));

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
			config.events_wanted = strtoul(optarg, &end, 10);
			if (*end != '\0')
				die("invalid number: %s", optarg);
			break;
		case 's':
			config.silent = true;
			break;
		case 'r':
			watch_rising = true;
			break;
		case 'f':
			watch_falling = true;
			break;
		case 'F':
			config.fmt = optarg;
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
		die("GPIO line offset must be specified");

	chip = gpiod_chip_open_lookup(argv[0]);
	if (!chip)
		die_perror("error opening gpiochip '%s'", argv[0]);

	evconf.consumer = "gpiomon";
	evconf.flags = 0;
	evconf.active_state = active_low ? GPIOD_REQUEST_ACTIVE_LOW
					 : GPIOD_REQUEST_ACTIVE_HIGH;

	if (watch_falling && !watch_rising)
		evconf.request_type = GPIOD_REQUEST_EVENT_FALLING_EDGE;
	else if (watch_rising && !watch_falling)
		evconf.request_type = GPIOD_REQUEST_EVENT_RISING_EDGE;
	else
		evconf.request_type = GPIOD_REQUEST_EVENT_BOTH_EDGES;

	for (i = 1; i < argc; i++) {
		offset = strtoul(argv[i], &end, 10);
		if (*end != '\0' || offset > INT_MAX)
			die("invalid GPIO offset: %s", argv[i]);

		line = gpiod_chip_get_line(chip, offset);
		if (!line)
			die_perror("error retrieving GPIO line from chip");

		rv = gpiod_line_request(line, &evconf, 0);
		if (rv)
			die_perror("error configuring GPIO line events");

		gpiod_line_bulk_add(&linebulk, line);
		num_lines++;
	}

	pfds = calloc(sizeof(struct pollfd), num_lines + 1);
	if (!pfds)
		die("out of memory");

	for (i = 0; i < num_lines; i++) {
		pfds[i].fd = gpiod_line_event_get_fd(linebulk.lines[i]);
		pfds[i].events = POLLIN | POLLPRI;
	}

	sigfd = make_signalfd();
	pfds[i].fd = sigfd;
	pfds[i].events = POLLIN | POLLPRI;

	do {
		numev = poll(pfds, num_lines + 1, 10000);
		if (numev < 0)
			die("error polling for events: %s", strerror(errno));
		else if (numev == 0)
			continue;

		for (i = 0, evdone = 0; i < num_lines; i++) {
			if (!pfds[i].revents)
				continue;

			rv = gpiod_line_event_read(linebulk.lines[i], &evbuf);
			if (rv)
				die_perror("error reading line event");

			handle_event(gpiod_line_offset(linebulk.lines[i]),
						       &evbuf, &config);
			evdone++;

			if (config.stop || evdone == numev)
				break;
		}

		/*
		 * There's a signal pending. No need to read it, we know we
		 * should quit now.
		 */
		if (pfds[num_lines].revents) {
			close(sigfd);
			config.stop = true;
		}
	} while (!config.stop);

	free(pfds);
	gpiod_chip_close(chip);

	return EXIT_SUCCESS;
}
