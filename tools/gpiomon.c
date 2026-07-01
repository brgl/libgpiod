// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>
// SPDX-FileCopyrightText: 2022 Kent Gibson <warthog618@gmail.com>

#include <getopt.h>
#include <gpiod.h>
#include <inttypes.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tools-common.h"

#define EVENT_BUF_SIZE 32

struct config {
	bool active_low;
	bool banner;
	bool by_name;
	bool quiet;
	bool strict;
	bool unquoted;
	enum gpiod_line_bias bias;
	enum gpiod_line_edge edges;
	int events_wanted;
	unsigned long long debounce_period_us;
	const char *chip_id;
	const char *consumer;
	const char *fmt;
	enum gpiod_line_clock event_clock;
	int timestamp_fmt;
	long long idle_timeout;
	bool initial_state;
	bool numeric;
};

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <line>...\n", get_prog_name());
	printf("\n");
	printf("Wait for events on GPIO lines and print them to standard output.\n");
	printf("\n");
	printf("Lines are specified by name, or optionally by offset if the chip option\n");
	printf("is provided.\n");
	printf("\n");
	printf("Options:\n");
	printf("      --banner\t\tdisplay a banner on successful startup\n");
	print_bias_help();
	printf("      --by-name\t\ttreat lines as names even if they would parse as an offset\n");
	printf("  -c, --chip <chip>\trestrict scope to a particular chip\n");
	printf("  -C, --consumer <name>\tconsumer name applied to requested lines (default is 'gpiomon')\n");
	printf("  -e, --edges <edges>\tspecify the edges to monitor\n");
	printf("\t\t\tPossible values: 'falling', 'rising', 'both'.\n");
	printf("\t\t\t(default is 'both')\n");
	printf("  -E, --event-clock <clock>\n");
	printf("\t\t\tspecify the source clock for event timestamps\n");
	printf("\t\t\tPossible values: 'monotonic', 'realtime', 'hte'.\n");
	printf("\t\t\t(default is 'monotonic')\n");
	printf("\t\t\tBy default 'realtime' is formatted as UTC, others as raw u64.\n");
	printf("  -h, --help\t\tdisplay this help and exit\n");
	printf("  -F, --format <fmt>\tspecify a custom output format\n");
	printf("      --idle-timeout <period>\n");
	printf("\t\t\texit gracefully if no events occur for the period specified\n");
	printf("  -l, --active-low\ttreat the line as active low, flipping the sense of\n");
	printf("\t\t\trising and falling edges\n");
	printf("      --localtime\tformat event timestamps as local time\n");
	printf("  -n, --num-events <num>\n");
	printf("\t\t\texit after processing num events\n");
	printf("  -p, --debounce-period <period>\n");
	printf("\t\t\tdebounce the line(s) with the specified period\n");
	printf("  -q, --quiet\t\tdon't generate any output\n");
	printf("  -I, --initial-state\t\tprint initial states of the lines\n");
	printf("  -N, --numeric\t\tdisplay line values as '0' (inactive) or '1' (active)\n");
	printf("  -s, --strict\t\tabort if requested line names are not unique\n");
	printf("      --unquoted\tdon't quote line or consumer names\n");
	printf("      --utc\t\tformat event timestamps as UTC (default for 'realtime')\n");
	printf("  -v, --version\t\toutput version information and exit\n");
	print_chip_help();
	print_period_help();
	print_event_format_help(true);
}

static int parse_edges_or_die(const char *option)
{
	if (strcmp(option, "rising") == 0)
		return GPIOD_LINE_EDGE_RISING;
	if (strcmp(option, "falling") == 0)
		return GPIOD_LINE_EDGE_FALLING;
	if (strcmp(option, "both") != 0)
		die("invalid edges: %s", option);

	return GPIOD_LINE_EDGE_BOTH;
}

static int parse_event_clock_or_die(const char *option)
{
	if (strcmp(option, "realtime") == 0)
		return GPIOD_LINE_CLOCK_REALTIME;
	if (strcmp(option, "hte") == 0)
		return GPIOD_LINE_CLOCK_HTE;
	if (strcmp(option, "monotonic") != 0)
		die("invalid event clock: %s", option);

	return GPIOD_LINE_CLOCK_MONOTONIC;
}

static int parse_config(int argc, char **argv, struct config *cfg)
{
	static const char *const shortopts = "+b:c:C:e:E:hF:ln:p:qshvIN";

	const struct option longopts[] = {
		{ "active-low",	no_argument,	NULL,		'l' },
		{ "banner",	no_argument,	NULL,		'-'},
		{ "bias",	required_argument, NULL,	'b' },
		{ "by-name",	no_argument,	NULL,		'B'},
		{ "chip",	required_argument, NULL,	'c' },
		{ "consumer",	required_argument, NULL,	'C' },
		{ "debounce-period", required_argument, NULL,	'p' },
		{ "edges",	required_argument, NULL,	'e' },
		{ "event-clock", required_argument, NULL,	'E' },
		{ "format",	required_argument, NULL,	'F' },
		{ "help",	no_argument,	NULL,		'h' },
		{ "idle-timeout",	required_argument,	NULL,		'i' },
		{ "localtime",	no_argument,	&cfg->timestamp_fmt,	2 },
		{ "num-events",	required_argument, NULL,	'n' },
		{ "quiet",	no_argument,	NULL,		'q' },
		{ "silent",	no_argument,	NULL,		'q' },
		{ "strict",	no_argument,	NULL,		's' },
		{ "unquoted",	no_argument,	NULL,		'Q' },
		{ "utc",	no_argument,	&cfg->timestamp_fmt,	1 },
		{ "version",	no_argument,	NULL,		'v' },
		{ "initial-state",	no_argument,	NULL,		'I' },
		{ "numeric",	no_argument,	NULL,		'N' },
		{ GETOPT_NULL_LONGOPT },
	};

	int opti, optc;

	memset(cfg, 0, sizeof(*cfg));
	cfg->edges = GPIOD_LINE_EDGE_BOTH;
	cfg->consumer = "gpiomon";
	cfg->idle_timeout = -1;

	for (;;) {
		optc = getopt_long(argc, argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case '-':
			cfg->banner = true;
			break;
		case 'b':
			cfg->bias = parse_bias_or_die(optarg);
			break;
		case 'B':
			cfg->by_name = true;
			break;
		case 'c':
			cfg->chip_id = optarg;
			break;
		case 'C':
			cfg->consumer = optarg;
			break;
		case 'e':
			cfg->edges = parse_edges_or_die(optarg);
			break;
		case 'E':
			cfg->event_clock = parse_event_clock_or_die(optarg);
			break;
		case 'F':
			cfg->fmt = optarg;
			break;
		case 'i':
			cfg->idle_timeout = parse_period_or_die(optarg);
			break;
		case 'l':
			cfg->active_low = true;
			break;
		case 'n':
			cfg->events_wanted = parse_uint_or_die(optarg);
			break;
		case 'p':
			cfg->debounce_period_us = parse_period_or_die(optarg);
			break;
		case 'q':
			cfg->quiet = true;
			break;
		case 'Q':
			cfg->unquoted = true;
			break;
		case 's':
			cfg->strict = true;
			break;
		case 'I':
			cfg->initial_state = true;
			break;
		case 'N':
			cfg->numeric = true;
			break;
		case 'h':
			print_help();
			exit(EXIT_SUCCESS);
		case 'v':
			print_version();
			exit(EXIT_SUCCESS);
		case '?':
			die("try %s --help", get_prog_name());
		case 0:
			break;
		default:
			abort();
		}
	}

	/* setup default clock/format combinations, where not overridden */
	if (cfg->event_clock == 0) {
		if (cfg->timestamp_fmt)
			cfg->event_clock = GPIOD_LINE_CLOCK_REALTIME;
		else
			cfg->event_clock = GPIOD_LINE_CLOCK_MONOTONIC;
	} else if ((cfg->event_clock == GPIOD_LINE_CLOCK_REALTIME) &&
		   (cfg->timestamp_fmt == 0)) {
		cfg->timestamp_fmt = 1;
	}

	return optind;
}

static void print_banner(int num_lines, char **lines)
{
	int i;

	if (num_lines > 1) {
		printf("Monitoring lines ");

		for (i = 0; i < num_lines - 1; i++)
			printf("'%s', ", lines[i]);

		printf("and '%s'...\n", lines[i]);
	} else {
		printf("Monitoring line '%s'...\n", lines[0]);
	}
}

static void event_print(struct gpiod_edge_event *event,
			struct line_resolver *resolver, int chip_num,
			struct config *cfg)
{
	const char *line_name, *chip_name;
	unsigned int offset;

	if (cfg->quiet)
		return;

	offset = gpiod_edge_event_get_line_offset(event);
	line_name = get_line_name(resolver, chip_num, offset);
	chip_name = cfg->chip_id ? get_chip_name(resolver, chip_num) : NULL;

	print_event(gpiod_edge_event_get_timestamp_ns(event),
		    offset,
		    gpiod_edge_event_get_event_type(event),
		    cfg->timestamp_fmt,
		    cfg->fmt,
		    line_name,
		    chip_name,
		    cfg->unquoted);
}

int main(int argc, char **argv)
{
	struct gpiod_edge_event_buffer *event_buffer;
	struct gpiod_line_settings *settings;
	struct gpiod_request_config *req_cfg;
	struct gpiod_line_request **requests;
	struct gpiod_line_config *line_cfg;
	int num_lines, events_done = 0;
	struct gpiod_edge_event *event;
	struct line_resolver *resolver;
	enum gpiod_line_value *values;
	struct timespec idle_timeout;
	struct gpiod_chip *chip;
	struct pollfd *pollfds;
	unsigned int *offsets;
	struct config cfg;
	int ret, i, j;

	set_prog_name(argv[0]);
	i = parse_config(argc, argv, &cfg);
	argc -= i;
	argv += i;

	if (argc < 1)
		die("at least one GPIO line must be specified");

	if (argc > 64)
		die("too many lines given");

	settings = gpiod_line_settings_new();
	if (!settings)
		die_perror("unable to allocate line settings");

	if (cfg.bias)
		gpiod_line_settings_set_bias(settings, cfg.bias);

	if (cfg.active_low)
		gpiod_line_settings_set_active_low(settings, true);

	if (cfg.debounce_period_us) {
		if (cfg.debounce_period_us > UINT_MAX)
			die("maximum debounce period is %uus, got %lluus",
			    UINT_MAX, cfg.debounce_period_us);

		gpiod_line_settings_set_debounce_period_us(
			settings, (unsigned long)cfg.debounce_period_us);
	}

	gpiod_line_settings_set_event_clock(settings, cfg.event_clock);
	gpiod_line_settings_set_edge_detection(settings, cfg.edges);

	line_cfg = gpiod_line_config_new();
	if (!line_cfg)
		die_perror("unable to allocate the line config structure");

	req_cfg = gpiod_request_config_new();
	if (!req_cfg)
		die_perror("unable to allocate the request config structure");

	gpiod_request_config_set_consumer(req_cfg, cfg.consumer);

	event_buffer = gpiod_edge_event_buffer_new(EVENT_BUF_SIZE);
	if (!event_buffer)
		die_perror("unable to allocate the line event buffer");

	resolver = resolve_lines(argc, argv, cfg.chip_id, cfg.strict,
				 cfg.by_name);
	validate_resolution(resolver, cfg.chip_id);
	requests = calloc(resolver->num_chips, sizeof(*requests));
	pollfds = calloc(resolver->num_chips, sizeof(*pollfds));
	offsets = calloc(resolver->num_lines, sizeof(*offsets));

	if (!requests || !pollfds || !offsets)
		die_oom();

	for (i = 0; i < resolver->num_chips; i++) {
		num_lines = get_line_offsets_and_values(resolver, i, offsets,
							NULL);
		gpiod_line_config_reset(line_cfg);
		ret = gpiod_line_config_add_line_settings(line_cfg, offsets,
							  num_lines, settings);
		if (ret)
			die_perror("unable to add line settings");

		chip = gpiod_chip_open(resolver->chips[i].path);
		if (!chip)
			die_perror("unable to open chip '%s'",
				   resolver->chips[i].path);

		requests[i] = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
		if (!requests[i])
			die_perror("unable to request lines on chip %s",
				   resolver->chips[i].path);

		if (cfg.initial_state) {
			values = calloc(resolver->num_lines, sizeof(*values));
			if (!values)
				die_oom();

			ret = gpiod_line_request_get_values(requests[i], values);
			if (ret)
				die_perror("unable to read GPIO line values");

			set_line_values(resolver, i, values);
			print_line_vals(resolver, cfg.unquoted, cfg.numeric);
			free(values);
		}

		pollfds[i].fd = gpiod_line_request_get_fd(requests[i]);
		pollfds[i].events = POLLIN;
		gpiod_chip_close(chip);
	}

	gpiod_request_config_free(req_cfg);
	gpiod_line_config_free(line_cfg);
	gpiod_line_settings_free(settings);

	if (cfg.banner)
		print_banner(argc, argv);

	if (cfg.idle_timeout > 0) {
		idle_timeout.tv_sec = cfg.idle_timeout / 1000000;
		idle_timeout.tv_nsec =
				(cfg.idle_timeout % 1000000) * 1000;
	}

	for (;;) {
		fflush(stdout);

		ret = ppoll(pollfds, resolver->num_chips,
			    cfg.idle_timeout > 0 ? &idle_timeout : NULL, NULL);
		if (ret < 0)
			die_perror("error polling for events");

		if (ret == 0)
			goto done;

		for (i = 0; i < resolver->num_chips; i++) {
			if (pollfds[i].revents == 0)
				continue;

			ret = gpiod_line_request_read_edge_events(requests[i],
					 event_buffer, EVENT_BUF_SIZE);
			if (ret < 0)
				die_perror("error reading line events");

			for (j = 0; j < ret; j++) {
				event = gpiod_edge_event_buffer_get_event(
						event_buffer, j);
				if (!event)
					die_perror("unable to retrieve event from buffer");

				event_print(event, resolver, i, &cfg);

				events_done++;

				if (cfg.events_wanted &&
				    events_done >= cfg.events_wanted)
					goto done;
			}
		}
	}

done:
	for (i = 0; i < resolver->num_chips; i++)
		gpiod_line_request_release(requests[i]);

	free(requests);
	free_line_resolver(resolver);
	gpiod_edge_event_buffer_free(event_buffer);
	free(offsets);

	return EXIT_SUCCESS;
}
