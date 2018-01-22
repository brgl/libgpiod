/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
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
	{ GETOPT_NULL_LONGOPT },
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
	bool watch_rising;
	bool watch_falling;

	unsigned int offset;
	unsigned int events_wanted;
	unsigned int events_done;

	bool silent;
	char *fmt;

	int sigfd;
};

static void event_print_custom(unsigned int offset,
			       const struct timespec *ts,
			       int event_type,
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
			if (event_type == GPIOD_SIMPLE_EVENT_CB_RISING_EDGE)
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

static void event_print_human_readable(unsigned int offset,
				       const struct timespec *ts,
				       int event_type)
{
	char *evname;

	if (event_type == GPIOD_SIMPLE_EVENT_CB_RISING_EDGE)
		evname = " RISING EDGE";
	else
		evname = "FALLING EDGE";

	printf("event: %s offset: %u timestamp: [%8ld.%09ld]\n",
	       evname, offset, ts->tv_sec, ts->tv_nsec);
}

static int poll_callback(unsigned int num_lines,
			 struct gpiod_simple_event_poll_fd *fds,
			 const struct timespec *timeout, void *data)
{
	struct pollfd pfds[GPIOD_LINE_BULK_MAX_LINES + 1];
	struct mon_ctx *ctx = data;
	int cnt, ts, ret;
	unsigned int i;

	for (i = 0; i < num_lines; i++) {
		pfds[i].fd = fds[i].fd;
		pfds[i].events = POLLIN | POLLPRI;
	}

	pfds[i].fd = ctx->sigfd;
	pfds[i].events = POLLIN | POLLPRI;

	ts = timeout->tv_sec * 1000 + timeout->tv_nsec / 1000000;

	cnt = poll(pfds, num_lines + 1, ts);
	if (cnt < 0)
		return GPIOD_SIMPLE_EVENT_POLL_RET_ERR;
	else if (cnt == 0)
		return GPIOD_SIMPLE_EVENT_POLL_RET_TIMEOUT;

	ret = cnt;
	for (i = 0; i < num_lines; i++) {
		if (pfds[i].revents) {
			fds[i].event = true;
			if (!--cnt)
				return ret;
		}
	}

	/*
	 * If we're here, then there's a signal pending. No need to read it,
	 * we know we should quit now.
	 */
	close(ctx->sigfd);

	return GPIOD_SIMPLE_EVENT_POLL_RET_STOP;
}

static void handle_event(struct mon_ctx *ctx, int event_type,
			 unsigned int line_offset,
			 const struct timespec *timestamp)
{
	if (!ctx->silent) {
		if (ctx->fmt)
			event_print_custom(line_offset, timestamp,
					   event_type, ctx);
		else
			event_print_human_readable(line_offset,
						   timestamp, event_type);
	}

	ctx->events_done++;
}

static int event_callback(int event_type, unsigned int line_offset,
			  const struct timespec *timestamp, void *data)
{
	struct mon_ctx *ctx = data;

	switch (event_type) {
	case GPIOD_SIMPLE_EVENT_CB_RISING_EDGE:
		if (ctx->watch_rising)
			handle_event(ctx, event_type, line_offset, timestamp);
		break;
	case GPIOD_SIMPLE_EVENT_CB_FALLING_EDGE:
		if (ctx->watch_falling)
			handle_event(ctx, event_type, line_offset, timestamp);
		break;
	default:
		return GPIOD_SIMPLE_EVENT_CB_RET_OK;
	}

	if (ctx->events_wanted && ctx->events_done >= ctx->events_wanted)
		return GPIOD_SIMPLE_EVENT_CB_RET_STOP;

	return GPIOD_SIMPLE_EVENT_CB_RET_OK;
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
	unsigned int offsets[GPIOD_LINE_BULK_MAX_LINES];
	struct timespec timeout = { 10, 0 };
	unsigned int num_lines = 0, offset;
	bool active_low = false;
	int optc, opti, ret, i;
	struct mon_ctx ctx;
	char *end;

	memset(&ctx, 0, sizeof(ctx));

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
			ctx.events_wanted = strtoul(optarg, &end, 10);
			if (*end != '\0')
				die("invalid number: %s", optarg);
			break;
		case 's':
			ctx.silent = true;
			break;
		case 'r':
			ctx.watch_rising = true;
			break;
		case 'f':
			ctx.watch_falling = true;
			break;
		case 'F':
			ctx.fmt = optarg;
			break;
		case '?':
			die("try %s --help", get_progname());
		default:
			abort();
		}
	}

	argc -= optind;
	argv += optind;

	if (!ctx.watch_rising && !ctx.watch_falling)
		ctx.watch_rising = ctx.watch_falling = true;

	if (argc < 1)
		die("gpiochip must be specified");

	if (argc < 2)
		die("at least one GPIO line offset must be specified");

	for (i = 1; i < argc; i++) {
		offset = strtoul(argv[i], &end, 10);
		if (*end != '\0' || offset > INT_MAX)
			die("invalid GPIO offset: %s", argv[i]);

		offsets[i - 1] = offset;
		num_lines++;
	}

	ctx.sigfd = make_signalfd();

	ret = gpiod_simple_event_loop_multiple(argv[0], offsets, num_lines,
					       active_low, "gpiomon", &timeout,
					       poll_callback,
					       event_callback, &ctx);
	if (ret)
		die_perror("error waiting for events");

	return EXIT_SUCCESS;
}
