// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

#include <errno.h>
#include <getopt.h>
#include <gpiod.h>
#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tools-common.h"

#define EVENT_BUF_SIZE 32

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

static void event_print_custom(unsigned int offset, uint64_t timeout,
			       int event_type, struct mon_ctx *ctx)
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
			if (event_type == GPIOD_EDGE_EVENT_RISING_EDGE)
				fputc('1', stdout);
			else
				fputc('0', stdout);
			break;
		case 's':
			printf("%"PRIu64, timeout / 1000000000);
			break;
		case 'n':
			printf("%"PRIu64, timeout % 1000000000);
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
				       uint64_t timeout, int event_type)
{
	char *evname;

	if (event_type == GPIOD_EDGE_EVENT_RISING_EDGE)
		evname = " RISING EDGE";
	else
		evname = "FALLING EDGE";

	printf("event: %s offset: %u timestamp: [%8"PRIu64".%09"PRIu64"]\n",
	       evname, offset, timeout / 1000000000, timeout % 1000000000);
}

static void handle_event(unsigned int line_offset, unsigned int event_type,
			 uint64_t timestamp, struct mon_ctx *ctx)
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
	bool watch_rising = false, watch_falling = false, active_low = false;
	size_t num_lines = 0, events_wanted = 0, events_done = 0;
	struct gpiod_edge_event_buffer *event_buffer;
	int optc, opti, ret, i, edge, bias = 0;
	uint64_t timeout = 10 * 1000000000LLU;
	struct gpiod_line_settings *settings;
	struct gpiod_request_config *req_cfg;
	struct gpiod_line_request *request;
	struct gpiod_line_config *line_cfg;
	unsigned int offsets[64], offset;
	struct gpiod_edge_event *event;
	struct gpiod_chip *chip;
	struct mon_ctx ctx;
	char *end;

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
			active_low = true;
			break;
		case 'B':
			bias = parse_bias(optarg);
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
		edge = GPIOD_LINE_EDGE_RISING;
	else if (watch_falling && !watch_rising)
		edge = GPIOD_LINE_EDGE_FALLING;
	else
		edge = GPIOD_LINE_EDGE_BOTH;

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

	if (has_duplicate_offsets(num_lines, offsets))
		die("offsets must be unique");

	chip = chip_open_lookup(argv[0]);
	if (!chip)
		die_perror("unable to open %s", argv[0]);

	settings = gpiod_line_settings_new();
	if (!settings)
		die_perror("unable to allocate line settings");

	if (bias)
		gpiod_line_settings_set_bias(settings, bias);
	if (active_low)
		gpiod_line_settings_set_active_low(settings, active_low);
	gpiod_line_settings_set_edge_detection(settings, edge);

	req_cfg = gpiod_request_config_new();
	if (!req_cfg)
		die_perror("unable to allocate the request config structure");

	gpiod_request_config_set_consumer(req_cfg, "gpiomon");

	line_cfg = gpiod_line_config_new();
	if (!line_cfg)
		die_perror("unable to allocate the line config structure");

	ret = gpiod_line_config_add_line_settings(line_cfg, offsets,
						  num_lines, settings);
	if (ret)
		die_perror("unable to add line settings");

	request = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
	if (!request)
		die_perror("unable to request lines");

	event_buffer = gpiod_edge_event_buffer_new(EVENT_BUF_SIZE);
	if (!event_buffer)
		die_perror("unable to allocate the line event buffer");

	for (;;) {
		ret = gpiod_line_request_wait_edge_event(request, timeout);
		if (ret < 0)
			die_perror("error waiting for events");
		if (ret == 0)
			continue;

		ret = gpiod_line_request_read_edge_event(request, event_buffer,
							 EVENT_BUF_SIZE);
		if (ret < 0)
			die_perror("error reading line events");

		for (i = 0; i < ret; i++) {
			event = gpiod_edge_event_buffer_get_event(event_buffer,
								  i);
			if (!event)
				die_perror("unable to retrieve the event from the buffer");

			handle_event(gpiod_edge_event_get_line_offset(event),
				     gpiod_edge_event_get_event_type(event),
				     gpiod_edge_event_get_timestamp_ns(event),
				     &ctx);

			events_done++;

			if (events_wanted && events_done >= events_wanted)
				goto done;
		}
	}

done:
	gpiod_edge_event_buffer_free(event_buffer);
	gpiod_line_request_release(request);
	gpiod_request_config_free(req_cfg);
	gpiod_line_config_free(line_cfg);
	gpiod_line_settings_free(settings);
	gpiod_chip_close(chip);

	return EXIT_SUCCESS;
}
