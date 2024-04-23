// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2022 Kent Gibson <warthog618@gmail.com>

#include <getopt.h>
#include <gpiod.h>
#include <inttypes.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tools-common.h"

struct config {
	bool banner;
	bool by_name;
	bool quiet;
	bool strict;
	bool unquoted;
	int event_type;
	int events_wanted;
	const char *chip_id;
	const char *fmt;
	int timestamp_fmt;
	int idle_timeout;
};

static void print_help(void)
{
	printf("Usage: %s [OPTIONS] <line>...\n", get_prog_name());
	printf("\n");
	printf("Wait for changes to info on GPIO lines and print them to standard output.\n");
	printf("\n");
	printf("Lines are specified by name, or optionally by offset if the chip option\n");
	printf("is provided.\n");
	printf("\n");
	printf("Options:\n");
	printf("      --banner\t\tdisplay a banner on successful startup\n");
	printf("      --by-name\t\ttreat lines as names even if they would parse as an offset\n");
	printf("  -c, --chip <chip>\trestrict scope to a particular chip\n");
	printf("  -e, --event <event>\tspecify the events to monitor\n");
	printf("\t\t\tPossible values: 'requested', 'released', 'reconfigured'.\n");
	printf("\t\t\t(default is all events)\n");
	printf("  -h, --help\t\tdisplay this help and exit\n");
	printf("  -F, --format <fmt>\tspecify a custom output format\n");
	printf("      --idle-timeout <period>\n");
	printf("\t\t\texit gracefully if no events occur for the period specified\n");
	printf("      --localtime\tconvert event timestamps to local time\n");
	printf("  -n, --num-events <num>\n");
	printf("\t\t\texit after processing num events\n");
	printf("  -q, --quiet\t\tdon't generate any output\n");
	printf("  -s, --strict\t\tabort if requested line names are not unique\n");
	printf("      --unquoted\tdon't quote line or consumer names\n");
	printf("      --utc\t\tconvert event timestamps to UTC\n");
	printf("  -v, --version\t\toutput version information and exit\n");
	print_chip_help();
	print_period_help();
	printf("\n");
	printf("Format specifiers:\n");
	printf("  %%o   GPIO line offset\n");
	printf("  %%l   GPIO line name\n");
	printf("  %%c   GPIO chip name\n");
	printf("  %%e   numeric info event type ('1' - requested, '2' - released or '3' - reconfigured)\n");
	printf("  %%E   info event type ('requested', 'released' or 'reconfigured')\n");
	printf("  %%a   line attributes\n");
	printf("  %%C   consumer\n");
	printf("  %%S   event timestamp as seconds\n");
	printf("  %%U   event timestamp as UTC\n");
	printf("  %%L   event timestamp as local time\n");
}

static int parse_event_type_or_die(const char *option)
{
	if (strcmp(option, "requested") == 0)
		return GPIOD_INFO_EVENT_LINE_REQUESTED;
	if (strcmp(option, "released") == 0)
		return GPIOD_INFO_EVENT_LINE_RELEASED;
	if (strcmp(option, "reconfigured") != 0)
		die("invalid edge: %s", option);

	return GPIOD_INFO_EVENT_LINE_CONFIG_CHANGED;
}

static int parse_config(int argc, char **argv, struct config *cfg)
{
	static const char *const shortopts = "+c:e:hF:n:qshv";

	const struct option longopts[] = {
		{ "banner",	no_argument,	NULL,		'-'},
		{ "by-name",	no_argument,	NULL,		'B'},
		{ "chip",	required_argument, NULL,	'c' },
		{ "event",	required_argument, NULL,	'e' },
		{ "format",	required_argument, NULL,	'F' },
		{ "help",	no_argument,	NULL,		'h' },
		{ "idle-timeout",	required_argument,	NULL,		'i' },
		{ "localtime",	no_argument,	&cfg->timestamp_fmt, 2 },
		{ "num-events",	required_argument, NULL,	'n' },
		{ "quiet",	no_argument,	NULL,		'q' },
		{ "silent",	no_argument,	NULL,		'q' },
		{ "strict",	no_argument,	NULL,		's' },
		{ "unquoted",	no_argument,	NULL,		'Q' },
		{ "utc",	no_argument,	&cfg->timestamp_fmt, 1 },
		{ "version",	no_argument,	NULL,		'v' },
		{ GETOPT_NULL_LONGOPT },
	};

	int opti, optc;

	memset(cfg, 0, sizeof(*cfg));
	cfg->idle_timeout = -1;

	for (;;) {
		optc = getopt_long(argc, argv, shortopts, longopts, &opti);
		if (optc < 0)
			break;

		switch (optc) {
		case '-':
			cfg->banner = true;
			break;
		case 'B':
			cfg->by_name = true;
			break;
		case 'c':
			cfg->chip_id = optarg;
			break;
		case 'e':
			cfg->event_type = parse_event_type_or_die(optarg);
			break;
		case 'F':
			cfg->fmt = optarg;
			break;
		case 'i':
			cfg->idle_timeout = parse_period_or_die(optarg);
			break;
		case 'n':
			cfg->events_wanted = parse_uint_or_die(optarg);
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

	return optind;
}

static void print_banner(int num_lines, char **lines)
{
	int i;

	if (num_lines > 1) {
		printf("Watching lines ");

		for (i = 0; i < num_lines - 1; i++)
			printf("'%s', ", lines[i]);

		printf("and '%s'...\n", lines[i]);
	} else {
		printf("Watching line '%s'...\n", lines[0]);
	}
}

static void print_event_type(int evtype)
{
	switch (evtype) {
	case GPIOD_INFO_EVENT_LINE_REQUESTED:
		fputs("requested", stdout);
		break;
	case GPIOD_INFO_EVENT_LINE_RELEASED:
		fputs("released", stdout);
		break;
	case GPIOD_INFO_EVENT_LINE_CONFIG_CHANGED:
		fputs("reconfigured", stdout);
		break;
	default:
		fputs("unknown", stdout);
		break;
	}
}

/*
 * A convenience function to map clock monotonic to realtime, as uAPI only
 * supports CLOCK_MONOTONIC.
 *
 * Samples the realtime clock on either side of a monotonic sample and averages
 * the realtime samples to estimate the offset between the two clocks.
 * Any time shifts between the two realtime samples will result in the
 * monotonic time being mapped to the average of the before and after, so
 * half way between the old and new times.
 *
 * Any CPU suspension between the event being generated and converted will
 * result in the returned time being shifted by the period of suspension.
 */
static uint64_t monotonic_to_realtime(uint64_t evtime)
{
	uint64_t before, after, mono;
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	before = ts.tv_nsec + ((uint64_t)ts.tv_sec) * 1000000000;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	mono = ts.tv_nsec + ((uint64_t)ts.tv_sec) * 1000000000;

	clock_gettime(CLOCK_REALTIME, &ts);
	after = ts.tv_nsec + ((uint64_t)ts.tv_sec) * 1000000000;

	evtime += (after / 2 - mono + before / 2);

	return evtime;
}

static void event_print_formatted(struct gpiod_info_event *event,
				  struct line_resolver *resolver, int chip_num,
				  struct config *cfg)
{
	const char *lname, *prev, *curr, *consumer;
	struct gpiod_line_info *info;
	unsigned int offset;
	uint64_t evtime;
	int evtype;
	char fmt;

	info = gpiod_info_event_get_line_info(event);
	evtime = gpiod_info_event_get_timestamp_ns(event);
	evtype = gpiod_info_event_get_event_type(event);
	offset = gpiod_line_info_get_offset(info);

	for (prev = curr = cfg->fmt;;) {
		curr = strchr(curr, '%');
		if (!curr) {
			fputs(prev, stdout);
			break;
		}

		if (prev != curr)
			fwrite(prev, curr - prev, 1, stdout);

		fmt = *(curr + 1);

		switch (fmt) {
		case 'a':
			print_line_attributes(info, cfg->unquoted);
			break;
		case 'c':
			fputs(get_chip_name(resolver, chip_num), stdout);
			break;
		case 'C':
			if (!gpiod_line_info_is_used(info)) {
				consumer = "unused";
			} else {
				consumer = gpiod_line_info_get_consumer(info);
				if (!consumer)
					consumer = "kernel";
			}
			fputs(consumer, stdout);
			break;
		case 'e':
			printf("%d", evtype);
			break;
		case 'E':
			print_event_type(evtype);
			break;
		case 'l':
			lname = gpiod_line_info_get_name(info);
			if (!lname)
				lname = "unnamed";
			fputs(lname, stdout);
			break;
		case 'L':
			print_event_time(monotonic_to_realtime(evtime), 2);
			break;
		case 'o':
			printf("%u", offset);
			break;
		case 'S':
			print_event_time(evtime, 0);
			break;
		case 'U':
			print_event_time(monotonic_to_realtime(evtime), 1);
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

static void event_print_human_readable(struct gpiod_info_event *event,
				       struct line_resolver *resolver,
				       int chip_num, struct config *cfg)
{
	struct gpiod_line_info *info;
	unsigned int offset;
	uint64_t evtime;
	char *evname;
	int evtype;

	info = gpiod_info_event_get_line_info(event);
	evtime = gpiod_info_event_get_timestamp_ns(event);
	evtype = gpiod_info_event_get_event_type(event);
	offset = gpiod_line_info_get_offset(info);

	switch (evtype) {
	case GPIOD_INFO_EVENT_LINE_REQUESTED:
		evname = "requested";
		break;
	case GPIOD_INFO_EVENT_LINE_RELEASED:
		evname = "released";
		break;
	case GPIOD_INFO_EVENT_LINE_CONFIG_CHANGED:
		evname = "reconfigured";
		break;
	default:
		evname = "unknown";
	}

	if (cfg->timestamp_fmt)
		evtime = monotonic_to_realtime(evtime);

	print_event_time(evtime, cfg->timestamp_fmt);
	printf("\t%s\t", evname);
	print_line_id(resolver, chip_num, offset, cfg->chip_id, cfg->unquoted);
	fputc('\n', stdout);
}

static void event_print(struct gpiod_info_event *event,
			struct line_resolver *resolver, int chip_num,
			struct config *cfg)
{
	if (cfg->quiet)
		return;

	if (cfg->fmt)
		event_print_formatted(event, resolver, chip_num, cfg);
	else
		event_print_human_readable(event, resolver, chip_num, cfg);
}

int main(int argc, char **argv)
{
	int i, j, ret, events_done = 0, evtype;
	struct line_resolver *resolver;
	struct gpiod_info_event *event;
	struct timespec idle_timeout;
	struct gpiod_chip **chips;
	struct gpiod_chip *chip;
	struct pollfd *pollfds;
	struct config cfg;

	set_prog_name(argv[0]);
	i = parse_config(argc, argv, &cfg);
	argc -= optind;
	argv += optind;

	if (argc < 1)
		die("at least one GPIO line must be specified");

	if (argc > 64)
		die("too many lines given");

	resolver = resolve_lines(argc, argv, cfg.chip_id, cfg.strict,
				 cfg.by_name);
	validate_resolution(resolver, cfg.chip_id);
	chips = calloc(resolver->num_chips, sizeof(*chips));
	pollfds = calloc(resolver->num_chips, sizeof(*pollfds));
	if (!pollfds)
		die("out of memory");

	for (i = 0; i < resolver->num_chips; i++) {
		chip = gpiod_chip_open(resolver->chips[i].path);
		if (!chip)
			die_perror("unable to open chip '%s'",
				   resolver->chips[i].path);

		for (j = 0; j < resolver->num_lines; j++)
			if ((resolver->lines[j].chip_num == i) &&
			    !gpiod_chip_watch_line_info(
				    chip, resolver->lines[j].offset))
				die_perror("unable to watch line on chip '%s'",
					   resolver->chips[i].path);

		chips[i] = chip;
		pollfds[i].fd = gpiod_chip_get_fd(chip);
		pollfds[i].events = POLLIN;
	}

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

			event = gpiod_chip_read_info_event(chips[i]);
			if (!event)
				die_perror("unable to retrieve chip event");

			if (cfg.event_type) {
				evtype = gpiod_info_event_get_event_type(event);
				if (evtype != cfg.event_type)
					continue;
			}

			event_print(event, resolver, i, &cfg);

			events_done++;

			if (cfg.events_wanted &&
			    events_done >= cfg.events_wanted)
				goto done;
		}
	}
done:
	for (i = 0; i < resolver->num_chips; i++)
		gpiod_chip_close(chips[i]);

	free(chips);
	free_line_resolver(resolver);

	return EXIT_SUCCESS;
}
