// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>
// SPDX-FileCopyrightText: 2022 Kent Gibson <warthog618@gmail.com>

/* Common code for GPIO tools. */

#include <errno.h>
#include <gpiod.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tools-common.h"

static const char *prog_name = NULL;
static const char *prog_short_name = NULL;

void set_prog_name(const char *name)
{
	prog_name = name;
	prog_short_name = name;
	while (*name) {
		if (*name++ == '/') {
			prog_short_name = name;
		}
	}
}

const char *get_prog_name(void)
{
	return prog_name;
}

const char *get_prog_short_name(void)
{
	return prog_short_name;
}

void print_error(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "%s: ", get_prog_name());
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
}

void print_perror(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "%s: ", get_prog_name());
	vfprintf(stderr, fmt, va);
	fprintf(stderr, ": %s\n", strerror(errno));
	va_end(va);
}

void die(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "%s: ", get_prog_name());
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);

	exit(EXIT_FAILURE);
}

void die_oom(void)
{
	die("out of memory");
}

void die_perror(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "%s: ", get_prog_name());
	vfprintf(stderr, fmt, va);
	fprintf(stderr, ": %s\n", strerror(errno));
	va_end(va);

	exit(EXIT_FAILURE);
}

void print_version(void)
{
	printf("%s v%s (libgpiod v%s)\n",
	       get_prog_short_name(), GPIOD_VERSION_STR, gpiod_api_version());
	printf("Copyright (C) 2017-2026 Bartosz Golaszewski\n");
	printf("License: GPL-2.0-or-later\n");
	printf("This is free software: you are free to change and redistribute it.\n");
	printf("There is NO WARRANTY, to the extent permitted by law.\n");
}

int parse_bias_or_die(const char *option)
{
	if (strcmp(option, "pull-down") == 0)
		return GPIOD_LINE_BIAS_PULL_DOWN;
	if (strcmp(option, "pull-up") == 0)
		return GPIOD_LINE_BIAS_PULL_UP;
	if (strcmp(option, "disabled") != 0)
		die("invalid bias: %s", option);

	return GPIOD_LINE_BIAS_DISABLED;
}

static int parse_line_value(const char *option)
{
	if (strcmp(option, "0") == 0 || strcmp(option, "inactive") == 0 ||
	    strcmp(option, "off") == 0 || strcmp(option, "false") == 0)
		return GPIOD_LINE_VALUE_INACTIVE;
	if (strcmp(option, "1") == 0 || strcmp(option, "active") == 0 ||
	    strcmp(option, "on") == 0 || strcmp(option, "true") == 0)
		return GPIOD_LINE_VALUE_ACTIVE;

	return GPIOD_LINE_VALUE_ERROR;
}

int parse_line_value_or_die(const char *option)
{
	int val = parse_line_value(option);

	if (val == GPIOD_LINE_VALUE_ERROR)
		die("invalid line value: '%s'", option);

	return val;
}

/*
 * Parse line id and value pairs from lvs into lines and values.
 *
 * Accepted forms:
 *     'line=value'
 *     '"line"=value'
 *
 * If line id is quoted it is returned unquoted.
 */
bool parse_line_values(int num_lines, char **lvs, char **lines,
		       enum gpiod_line_value *values)
{
	char *value, *line;
	int i;

	for (i = 0; i < num_lines; i++) {
		line = lvs[i];

		if (*line != '"') {
			value = strchr(line, '=');
		} else {
			line++;
			value = strstr(line, "\"=");
			if (value) {
				*value = '\0';
				value++;
			}
		}

		if (!value) {
			print_error("invalid line value: '%s'", lvs[i]);
			return false;
		}

		*value = '\0';
		value++;
		values[i] = parse_line_value(value);

		if (values[i] == GPIOD_LINE_VALUE_ERROR) {
			print_error("invalid line value: '%s'", value);
			return false;
		}

		lines[i] = line;
	}

	return true;
}

void parse_line_values_or_die(int num_lines, char **lvs, char **lines,
			      enum gpiod_line_value *values)
{
	if (!parse_line_values(num_lines, lvs, lines, values))
		exit(EXIT_FAILURE);
}

long long parse_period(const char *option)
{
	unsigned long long p, m = 0;
	char *end;

	p = strtoull(option, &end, 10);

	switch (*end) {
	case 'u':
		if (*(end + 1) != 's')
			return -1;
		m = 1;
		end++;
		break;
	case 'm':
		m = 1000;
		end++;
		break;
	case 's':
		m = 1000000;
		break;
	case '\0':
		break;
	default:
		return -1;
	}

	if (m) {
		if (*end == '\0')
			m = 60000000;
		else if (*end == 's')
			end++;
		else
			return -1;
	} else {
		m = 1000;
	}

	if (*end != '\0')
		return -1;

	if (m != 0 && p > ULLONG_MAX / m)
		return -1;
	p *= m;

	return p;
}

unsigned long long parse_period_or_die(const char *option)
{
	long long period = parse_period(option);

	if (period < 0)
		die("invalid period: %s", option);

	return period;
}

void sleep_us(unsigned long long period)
{
	struct timespec	spec;

	spec.tv_sec = period / 1000000;
	spec.tv_nsec = (period % 1000000) * 1000;

	nanosleep(&spec, NULL);
}

int parse_uint(const char *option)
{
	unsigned long o;
	char *end;

	o = strtoul(option, &end, 10);
	if (*end == '\0' && o <= INT_MAX)
		return o;

	return -1;
}

unsigned int parse_uint_or_die(const char *option)
{
	int i = parse_uint(option);

	if (i < 0)
		die("invalid number: '%s'", option);

	return i;
}

void print_bias_help(void)
{
	printf("  -b, --bias <bias>\tspecify the line bias\n");
	printf("\t\t\tPossible values: 'pull-down', 'pull-up', 'disabled'.\n");
	printf("\t\t\t(default is to leave bias unchanged)\n");
}

void print_chip_help(void)
{
	printf("\nChips:\n");
	printf("    A GPIO chip may be identified by number, name, or path.\n");
	printf("    e.g. '0', 'gpiochip0', and '/dev/gpiochip0' all refer to the same chip.\n");
}

void print_period_help(void)
{
	printf("\nPeriods:\n");
	printf("    Periods are taken as milliseconds unless units are specified. e.g. 10us.\n");
	printf("    Supported units are 'm', 's', 'ms', and 'us' for minutes, seconds, milliseconds and\n");
	printf("    microseconds respectively.\n");
}

void print_event_format_help(bool include_line_chip)
{
	printf("\n");
	printf("Format specifiers:\n");
	printf("  %%o   GPIO line offset\n");
	if (include_line_chip) {
		printf("  %%l   GPIO line name\n");
		printf("  %%c   GPIO chip name\n");
	}
	printf("  %%e   numeric edge event type ('1' - rising or '2' - falling)\n");
	printf("  %%E   edge event type ('rising' or 'falling')\n");
	printf("  %%S   event timestamp as seconds\n");
	printf("  %%U   event timestamp as UTC\n");
	printf("  %%L   event timestamp as local time\n");
}

#define TIME_BUFFER_SIZE 20

/*
 * format:
 * 0: raw seconds
 * 1: utc time
 * 2: local time
 */
void print_event_time(uint64_t evtime, int format)
{
	char tbuf[TIME_BUFFER_SIZE];
	time_t evtsec;
	struct tm t;
	char *tz;

	if (format) {
		evtsec = evtime / 1000000000;
		if (format == 2) {
			localtime_r(&evtsec, &t);
			tz = "";
		} else {
			gmtime_r(&evtsec, &t);
			tz = "Z";
		}
		strftime(tbuf, TIME_BUFFER_SIZE, "%FT%T", &t);
		printf("%s.%09" PRIu64 "%s", tbuf, evtime % 1000000000, tz);
	} else {
		printf("%" PRIu64 ".%09" PRIu64, evtime / 1000000000,
		       evtime % 1000000000);
	}
}

void print_event(uint64_t timestamp_ns, unsigned int offset, int event_type,
		 int timestamp_fmt, const char *fmt,
		 const char *line_name, const char *chip_name, bool unquoted)
{
	const char *prev, *curr;
	char spec;

	if (!fmt) {
		print_event_time(timestamp_ns, timestamp_fmt);
		if (event_type == GPIOD_EDGE_EVENT_RISING_EDGE)
			fputs("\trising\t", stdout);
		else
			fputs("\tfalling\t", stdout);
		if (!line_name) {
			if (chip_name)
				printf("%s %u", chip_name, offset);
			else
				printf("%u", offset);
		} else {
			if (chip_name)
				printf("%s %u ", chip_name, offset);
			printf(unquoted ? "%s" : "\"%s\"", line_name);
		}
		fputc('\n', stdout);
		return;
	}

	for (prev = curr = fmt;;) {
		curr = strchr(curr, '%');
		if (!curr) {
			fputs(prev, stdout);
			break;
		}

		if (prev != curr)
			fwrite(prev, curr - prev, 1, stdout);

		spec = *(curr + 1);

		switch (spec) {
		case 'c':
			fputs(chip_name ? chip_name : "unknown", stdout);
			break;
		case 'e':
			printf("%d", event_type);
			break;
		case 'E':
			fputs(event_type == GPIOD_EDGE_EVENT_RISING_EDGE ?
			      "rising" : "falling", stdout);
			break;
		case 'l':
			fputs(line_name ? line_name : "unnamed", stdout);
			break;
		case 'L':
			print_event_time(timestamp_ns, 2);
			break;
		case 'o':
			printf("%u", offset);
			break;
		case 'S':
			print_event_time(timestamp_ns, 0);
			break;
		case 'U':
			print_event_time(timestamp_ns, 1);
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

static void print_bias(struct gpiod_line_info *info)
{
	const char *name;

	switch (gpiod_line_info_get_bias(info)) {
	case GPIOD_LINE_BIAS_PULL_UP:
		name = "pull-up";
		break;
	case GPIOD_LINE_BIAS_PULL_DOWN:
		name = "pull-down";
		break;
	case GPIOD_LINE_BIAS_DISABLED:
		name = "disabled";
		break;
	default:
		return;
	}

	printf(" bias=%s", name);
}

static void print_drive(struct gpiod_line_info *info)
{
	const char *name;

	switch (gpiod_line_info_get_drive(info)) {
	case GPIOD_LINE_DRIVE_OPEN_DRAIN:
		name = "open-drain";
		break;
	case GPIOD_LINE_DRIVE_OPEN_SOURCE:
		name = "open-source";
		break;
	default:
		return;
	}

	printf(" drive=%s", name);
}

static void print_edge_detection(struct gpiod_line_info *info)
{
	const char *name;

	switch (gpiod_line_info_get_edge_detection(info)) {
	case GPIOD_LINE_EDGE_BOTH:
		name = "both";
		break;
	case GPIOD_LINE_EDGE_RISING:
		name = "rising";
		break;
	case GPIOD_LINE_EDGE_FALLING:
		name = "falling";
		break;
	default:
		return;
	}

	printf(" edges=%s", name);
}

static void print_event_clock(struct gpiod_line_info *info)
{
	const char *name;

	switch (gpiod_line_info_get_event_clock(info)) {
	case GPIOD_LINE_CLOCK_REALTIME:
		name = "realtime";
		break;
	case GPIOD_LINE_CLOCK_HTE:
		name = "hte";
		break;
	default:
		return;
	}

	printf(" event-clock=%s", name);
}

static void print_debounce(struct gpiod_line_info *info)
{
	const char *units = "us";
	unsigned long debounce;

	debounce = gpiod_line_info_get_debounce_period_us(info);
	if (!debounce)
		return;
	if (debounce % 1000000 == 0) {
		debounce /= 1000000;
		units = "s";
	} else if (debounce % 1000 == 0) {
		debounce /= 1000;
		units = "ms";
	}
	printf(" debounce-period=%lu%s", debounce, units);
}

static void print_consumer(struct gpiod_line_info *info, bool unquoted)
{
	const char *consumer;
	const char *fmt;

	if (!gpiod_line_info_is_used(info))
		return;

	consumer = gpiod_line_info_get_consumer(info);
	if (!consumer)
		consumer = "kernel";

	fmt = unquoted ? " consumer=%s" : " consumer=\"%s\"";

	printf(fmt, consumer);
}

void print_line_attributes(struct gpiod_line_info *info, bool unquoted_strings)
{
	enum gpiod_line_direction direction;

	direction = gpiod_line_info_get_direction(info);

	printf("%s", direction == GPIOD_LINE_DIRECTION_INPUT ?
			"input" : "output");

	if (gpiod_line_info_is_active_low(info))
		printf(" active-low");

	print_drive(info);
	print_bias(info);
	print_edge_detection(info);
	print_event_clock(info);
	print_debounce(info);
	print_consumer(info, unquoted_strings);
}

void print_line_id(struct line_resolver *resolver, int chip_num,
		   unsigned int offset, const char *chip_id, bool unquoted)
{
	const char *lname, *fmt;

	lname = get_line_name(resolver, chip_num, offset);
	if (!lname) {
		printf("%s %u", get_chip_name(resolver, chip_num), offset);
		return;
	}
	if (chip_id)
		printf("%s %u ", get_chip_name(resolver, chip_num), offset);

	fmt = unquoted ? "%s" : "\"%s\"";
	printf(fmt, lname);
}

void print_line_vals(struct line_resolver *resolver, bool is_unquoted,
		     bool is_numeric)
{
	const char *fmt = is_unquoted ? "%s=%s" : "\"%s\"=%s";
	int i;

	for (i = 0; i < resolver->num_lines; i++) {
		struct resolved_line *line = &resolver->lines[i];

		if (is_numeric)
			printf("%d", line->value);
		else
			printf(fmt, line->id,
			       line->value ? "active" : "inactive");

		if (i != resolver->num_lines - 1)
			printf(" ");
	}

	printf("\n");
}

bool chip_path_lookup(const char *id, char **path_ptr)
{
	char *path;

	path = gpiotools_chip_path_lookup(id);
	if (path)
		*path_ptr = path;

	return !!path;
}

int chip_paths(const char *id, char ***paths_ptr)
{
	int ret;

	ret = gpiotools_chip_paths(id, paths_ptr);
	if (ret < 0)
		die_oom();

	return ret;
}

int all_chip_paths(char ***paths_ptr)
{
	int ret;

	ret = gpiotools_all_chip_paths(paths_ptr);
	if (ret < 0)
		die_perror("unable to scan /dev");

	return ret;
}

/*
 * check for lines that can be identified by offset
 *
 * This only applies to the first chip, as otherwise the lines must be
 * identified by name.
 */
bool resolve_lines_by_offset(struct line_resolver *resolver,
			     unsigned int num_lines)
{
	return gpiotools_resolve_lines_by_offset(resolver, num_lines);
}

bool resolve_done(struct line_resolver *resolver)
{
	return gpiotools_resolve_done(resolver);
}

struct line_resolver *resolver_init(int num_lines, char **lines, int num_chips,
				    bool strict, bool by_name)
{
	struct line_resolver *resolver;

	resolver = gpiotools_resolver_init(num_lines, lines, num_chips,
					   strict, by_name);
	if (!resolver)
		die_oom();

	return resolver;
}

struct line_resolver *resolve_lines(int num_lines, char **lines,
				    const char *chip_id, bool strict,
				    bool by_name)
{
	struct line_resolver *resolver;
	int i;

	resolver = gpiotools_resolve_lines(num_lines, lines, chip_id,
					   strict, by_name);
	if (!resolver) {
		if (errno == ENODEV)
			die("cannot find GPIO chip character device '%s'",
			    chip_id);
		else
			die_perror("error resolving lines");
	}

	for (i = 0; i < resolver->num_lines; i++) {
		if (resolver->lines[i].not_unique)
			die("line '%s' is not unique", resolver->lines[i].id);
	}

	return resolver;
}

void validate_resolution(struct line_resolver *resolver, const char *chip_id)
{
	struct resolved_line *line, *prev;
	bool valid = true;
	int i, j;

	for (i = 0; i < resolver->num_lines; i++) {
		line = &resolver->lines[i];
		if (line->resolved) {
			for (j = 0; j < i; j++) {
				prev = &resolver->lines[j];
				if (prev->resolved &&
				    (prev->chip_num == line->chip_num) &&
				    (prev->offset == line->offset)) {
					print_error("lines '%s' and '%s' are the same line",
						    prev->id, line->id);
					valid = false;
					break;
				}
			}
			continue;
		}
		valid = false;
		if (chip_id && line->id_as_offset != -1)
			print_error("offset %s is out of range on chip '%s'",
				    line->id, chip_id);
		else
			print_error("cannot find line '%s'", line->id);
	}
	if (!valid)
		exit(EXIT_FAILURE);
}

void free_line_resolver(struct line_resolver *resolver)
{
	gpiotools_free_line_resolver(resolver);
}

int get_line_offsets_and_values(struct line_resolver *resolver, int chip_num,
				unsigned int *offsets,
				enum gpiod_line_value *values)
{
	return gpiotools_get_line_offsets_and_values(resolver, chip_num,
						     offsets, values);
}

const char *get_chip_name(struct line_resolver *resolver, int chip_num)
{
	return gpiotools_get_chip_name(resolver, chip_num);
}

const char *get_line_name(struct line_resolver *resolver, int chip_num,
			  unsigned int offset)
{
	return gpiotools_get_line_name(resolver, chip_num, offset);
}

void set_line_values(struct line_resolver *resolver, int chip_num,
		     enum gpiod_line_value *values)
{
	gpiotools_set_line_values(resolver, chip_num, values);
}
