// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <errno.h>
#include <getopt.h>
#include <gpiod.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "tools-common.h"

static const struct option longopts[] = {
	{ "help",		no_argument,		NULL,	'h' },
	{ "version",		no_argument,		NULL,	'v' },
	{ "active-low",		no_argument,		NULL,	'l' },
	{ "bias",		required_argument,	NULL,	'B' },
	{ "num-events",		required_argument,	NULL,	'n' },
	{ "silent",		no_argument,		NULL,	's' },
	{ "rising-edge",	no_argument,		NULL,	'r' },
	{ "falling-edge",	no_argument,		NULL,	'f' },
	{ "line-buffered",	no_argument,		NULL,	'b' },
	{ "format",		required_argument,	NULL,	'F' },
	{ GETOPT_NULL_LONGOPT },
};

static const char *const shortopts = "+hvlB:n:srfbF:";

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <chip name/number> <offset 1> <offset 2> ...\n",
	       get_progname());
	printf("\n");
	printf("Wait for events on GPIO lines and print them to standard output\n");
	printf("\n");
	printf("Options:\n");
	printf("  -h, --help:\t\tdisplay this message and exit\n");
	printf("  -v, --version:\tdisplay the version and exit\n");
	printf("  -l, --active-low:\tset the line active state to low\n");
	printf("  -B, --bias=[as-is|disable|pull-down|pull-up] (defaults to 'as-is'):\n");
	printf("		set the line bias\n");
	printf("  -n, --num-events=NUM:\texit after processing NUM events\n");
	printf("  -s, --silent:\t\tdon't print event info\n");
	printf("  -r, --rising-edge:\tonly process rising edge events\n");
	printf("  -f, --falling-edge:\tonly process falling edge events\n");
	printf("  -b, --line-buffered:\tset standard output as line buffered\n");
	printf("  -F, --format=FMT\tspecify custom output format\n");
	printf("\n");
	print_bias_help();
	printf("\n");
	printf("Format specifiers:\n");
	printf("  %%o:  GPIO line offset\n");
	printf("  %%e:  event type (0 - falling edge, 1 rising edge)\n");
	printf("  %%s:  seconds part of the event timestamp\n");
	printf("  %%n:  nanoseconds part of the event timestamp\n");
}

struct mon_ctx {
	unsigned int offset;
	bool silent;
	char *fmt;
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
			if (event_type == GPIOD_LINE_EVENT_RISING_EDGE)
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

	if (event_type == GPIOD_LINE_EVENT_RISING_EDGE)
		evname = " RISING EDGE";
	else
		evname = "FALLING EDGE";

	printf("event: %s offset: %u timestamp: [%8ld.%09ld]\n",
	       evname, offset, ts->tv_sec, ts->tv_nsec);
}

static void handle_event(unsigned int line_offset, unsigned int event_type,
			 struct timespec *timestamp, struct mon_ctx *ctx)
{
	if (!ctx->silent) {
		if (ctx->fmt)
			event_print_custom(line_offset, timestamp,
					   event_type, ctx);
		else
			event_print_human_readable(line_offset,
						   timestamp, event_type);
	}
}

static void handle_signal(int signum UNUSED)
{
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	unsigned int offsets[64], num_lines = 0, offset,
		     events_wanted = 0, events_done = 0, x;
	bool watch_rising = false, watch_falling = false;
	int flags = 0;
	struct timespec timeout = { 10, 0 };
	int optc, opti, rv, i, y, event_type;
	struct mon_ctx ctx;
	struct gpiod_chip *chip;
	struct gpiod_line_bulk *lines, *evlines;
	char *end;
	struct gpiod_line_request_config config;
	struct gpiod_line *line;
	struct gpiod_line_event events[16];

	/*
	 * FIXME: use signalfd once the API has been converted to using a single file
	 * descriptor as provided by uAPI v2.
	 */
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

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
			flags |= GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;
			break;
		case 'B':
			flags |= bias_flags(optarg);
			break;
		case 'n':
			events_wanted = strtoul(optarg, &end, 10);
			if (*end != '\0')
				die("invalid number: %s", optarg);
			break;
		case 's':
			ctx.silent = true;
			break;
		case 'r':
			watch_rising = true;
			break;
		case 'f':
			watch_falling = true;
			break;
		case 'b':
			setlinebuf(stdout);
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

	if (watch_rising && !watch_falling)
		event_type = GPIOD_LINE_REQUEST_EVENT_RISING_EDGE;
	else if (watch_falling && !watch_rising)
		event_type = GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE;
	else
		event_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;

	if (argc < 1)
		die("gpiochip must be specified");

	if (argc < 2)
		die("at least one GPIO line offset must be specified");

	if (argc > 65)
		die("too many offsets given");

	for (i = 1; i < argc; i++) {
		offset = strtoul(argv[i], &end, 10);
		if (*end != '\0' || offset > INT_MAX)
			die("invalid GPIO offset: %s", argv[i]);

		offsets[i - 1] = offset;
		num_lines++;
	}

	chip = chip_open_lookup(argv[0]);
	if (!chip)
		die_perror("unable to open %s", argv[0]);

	lines = gpiod_chip_get_lines(chip, offsets, num_lines);
	if (!lines)
		die_perror("unable to retrieve GPIO lines from chip");

	memset(&config, 0, sizeof(config));

	config.consumer = "gpiomon";
	config.request_type = event_type;
	config.flags = flags;

	rv = gpiod_line_request_bulk(lines, &config, NULL);
	if (rv)
		die_perror("unable to request GPIO lines for events");

	evlines = gpiod_line_bulk_new(num_lines);
	if (!evlines)
		die("out of memory");

	for (;;) {
		gpiod_line_bulk_reset(evlines);
		rv = gpiod_line_event_wait_bulk(lines, &timeout, evlines);
		if (rv < 0)
			die_perror("error waiting for events");
		if (rv == 0)
			continue;

		num_lines = gpiod_line_bulk_num_lines(evlines);

		for (x = 0; x < num_lines; x++) {
			line = gpiod_line_bulk_get_line(evlines, x);

			rv = gpiod_line_event_read_multiple(line, events,
							    ARRAY_SIZE(events));
			if (rv < 0)
				die_perror("error reading line events");

			for (y = 0; y < rv; y++) {
				handle_event(gpiod_line_offset(line),
					     events[y].event_type,
					     &events[y].ts, &ctx);
				events_done++;

				if (events_wanted &&
				    events_done >= events_wanted)
					goto done;
			}
		}
	}

done:
	gpiod_line_release_bulk(lines);
	gpiod_line_bulk_free(lines);
	gpiod_line_bulk_free(evlines);
	gpiod_chip_close(chip);

	return EXIT_SUCCESS;
}
