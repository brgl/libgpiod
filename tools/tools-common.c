// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>
// SPDX-FileCopyrightText: 2022 Kent Gibson <warthog618@gmail.com>

/* Common code for GPIO tools. */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <gpiod.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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
	printf("%s (libgpiod) v%s\n", get_prog_short_name(), gpiod_api_version());
	printf("Copyright (C) 2017-2023 Bartosz Golaszewski\n");
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

int parse_period(const char *option)
{
	unsigned long p, m = 0;
	char *end;

	p = strtoul(option, &end, 10);

	switch (*end) {
	case 'u':
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
		if (*end != 's')
			return -1;

		end++;
	} else {
		m = 1000;
	}

	p *= m;
	if (*end != '\0' || p > INT_MAX)
		return -1;

	return p;
}

unsigned int parse_period_or_die(const char *option)
{
	int period = parse_period(option);

	if (period < 0)
		die("invalid period: %s", option);

	return period;
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
	printf("    Supported units are 's', 'ms', and 'us'.\n");
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

static int chip_dir_filter(const struct dirent *entry)
{
	struct stat sb;
	int ret = 0;
	char *path;

	if (asprintf(&path, "/dev/%s", entry->d_name) < 0)
		return 0;

	if ((lstat(path, &sb) == 0) && (!S_ISLNK(sb.st_mode)) &&
	    gpiod_is_gpiochip_device(path))
		ret = 1;

	free(path);

	return ret;
}

static bool isuint(const char *str)
{
	for (; *str && isdigit(*str); str++)
		;

	return *str == '\0';
}

bool chip_path_lookup(const char *id, char **path_ptr)
{
	char *path;

	if (isuint(id)) {
		/* by number */
		if (asprintf(&path, "/dev/gpiochip%s", id) < 0)
			return false;
	} else if (strchr(id, '/')) {
		/* by path */
		if (asprintf(&path, "%s", id) < 0)
			return false;
	} else {
		/* by device name */
		if (asprintf(&path, "/dev/%s", id) < 0)
			return false;
	}

	if (!gpiod_is_gpiochip_device(path)) {
		free(path);
		return false;
	}

	*path_ptr = path;

	return true;
}

int chip_paths(const char *id, char ***paths_ptr)
{
	char **paths;
	char *path;

	if (id == NULL)
		return all_chip_paths(paths_ptr);

	if (!chip_path_lookup(id, &path))
		return 0;

	paths = malloc(sizeof(*paths));
	if (paths == NULL)
		die("out of memory");

	paths[0] = path;
	*paths_ptr = paths;

	return 1;
}

int all_chip_paths(char ***paths_ptr)
{
	int i, j, num_chips, ret = 0;
	struct dirent **entries;
	char **paths;

	num_chips = scandir("/dev/", &entries, chip_dir_filter, alphasort);
	if (num_chips < 0)
		die_perror("unable to scan /dev");

	paths = calloc(num_chips, sizeof(*paths));
	if (paths == NULL)
		die("out of memory");

	for (i = 0; i < num_chips; i++) {
		if (asprintf(&paths[i], "/dev/%s", entries[i]->d_name) < 0) {
			for (j = 0; j < i; j++)
				free(paths[j]);

			free(paths);
			return 0;
		}
	}

	*paths_ptr = paths;
	ret = num_chips;

	for (i = 0; i < num_chips; i++)
		free(entries[i]);

	free(entries);
	return ret;
}

static bool resolve_line(struct line_resolver *resolver,
			 struct gpiod_line_info *info, int chip_num)
{
	struct resolved_line *line;
	bool resolved = false;
	unsigned int offset;
	const char *name;
	int i;

	offset = gpiod_line_info_get_offset(info);
	for (i = 0; i < resolver->num_lines; i++) {
		line = &resolver->lines[i];
		/* already resolved by offset? */
		if (line->resolved && (line->offset == offset) &&
		    (line->chip_num == chip_num)) {
			line->info = info;
			resolver->num_found++;
			resolved = true;
		}
		if (line->resolved && !resolver->strict)
			continue;

		/* else resolve by name */
		name = gpiod_line_info_get_name(info);
		if (name && (strcmp(line->id, name) == 0)) {
			if (resolver->strict && line->resolved)
				die("line '%s' is not unique", line->id);
			line->offset = offset;
			line->info = info;
			line->chip_num = resolver->num_chips;
			line->resolved = true;
			resolver->num_found++;
			resolved = true;
		}
	}

	return resolved;
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
	struct resolved_line *line;
	bool used = false;
	int i;

	for (i = 0; i < resolver->num_lines; i++) {
		line = &resolver->lines[i];
		if ((line->id_as_offset != -1) &&
		    (line->id_as_offset < (int)num_lines)) {
			line->chip_num = 0;
			line->offset = line->id_as_offset;
			line->resolved = true;
			used = true;
		}
	}
	return used;
}

bool resolve_done(struct line_resolver *resolver)
{
	return (!resolver->strict &&
		resolver->num_found >= resolver->num_lines);
}

struct line_resolver *resolver_init(int num_lines, char **lines, int num_chips,
				    bool strict, bool by_name)
{
	struct line_resolver *resolver;
	struct resolved_line *line;
	size_t resolver_size;
	int i;

	resolver_size = sizeof(*resolver) + num_lines * sizeof(*line);
	resolver = malloc(resolver_size);
	if (resolver == NULL)
		die("out of memory");

	memset(resolver, 0, resolver_size);

	resolver->chips = calloc(num_chips, sizeof(struct resolved_chip));
	if (resolver->chips == NULL)
		die("out of memory");
	memset(resolver->chips, 0, num_chips * sizeof(struct resolved_chip));

	resolver->num_lines = num_lines;
	resolver->strict = strict;
	for (i = 0; i < num_lines; i++) {
		line = &resolver->lines[i];
		line->id = lines[i];
		line->id_as_offset = by_name ? -1 : parse_uint(lines[i]);
		line->chip_num = -1;
	}

	return resolver;
}

struct line_resolver *resolve_lines(int num_lines, char **lines,
				    const char *chip_id, bool strict,
				    bool by_name)
{
	struct gpiod_chip_info *chip_info;
	struct gpiod_line_info *line_info;
	struct line_resolver *resolver;
	int num_chips, i, offset;
	struct gpiod_chip *chip;
	bool chip_used;
	char **paths;

	if (chip_id == NULL)
		by_name = true;

	num_chips = chip_paths(chip_id, &paths);
	if (chip_id && (num_chips == 0))
		die("cannot find GPIO chip character device '%s'", chip_id);

	resolver = resolver_init(num_lines, lines, num_chips, strict, by_name);

	for (i = 0; (i < num_chips) && !resolve_done(resolver); i++) {
		chip_used = false;
		chip = gpiod_chip_open(paths[i]);
		if (!chip) {
			if ((errno == EACCES) && (chip_id == NULL)) {
				free(paths[i]);
				continue;
			}

			die_perror("unable to open chip '%s'", paths[i]);
		}

		chip_info = gpiod_chip_get_info(chip);
		if (!chip_info)
			die_perror("unable to get info for '%s'", paths[i]);

		num_lines = gpiod_chip_info_get_num_lines(chip_info);

		if (i == 0 && chip_id && !by_name)
			chip_used = resolve_lines_by_offset(resolver, num_lines);

		for (offset = 0;
		     (offset < num_lines) && !resolve_done(resolver);
		     offset++) {
			line_info = gpiod_chip_get_line_info(chip, offset);
			if (!line_info)
				die_perror("unable to read the info for line %d from %s",
					   offset,
					   gpiod_chip_info_get_name(chip_info));

			if (resolve_line(resolver, line_info, i))
				chip_used = true;
			else
				gpiod_line_info_free(line_info);

		}

		gpiod_chip_close(chip);

		if (chip_used) {
			resolver->chips[resolver->num_chips].info = chip_info;
			resolver->chips[resolver->num_chips].path = paths[i];
			resolver->num_chips++;
		} else {
			gpiod_chip_info_free(chip_info);
			free(paths[i]);
		}
	}
	free(paths);

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
	int i;

	if (!resolver)
		return;

	for (i = 0; i < resolver->num_lines; i++)
		gpiod_line_info_free(resolver->lines[i].info);

	for (i = 0; i < resolver->num_chips; i++) {
		gpiod_chip_info_free(resolver->chips[i].info);
		free(resolver->chips[i].path);
	}

	free(resolver->chips);
	free(resolver);
}

int get_line_offsets_and_values(struct line_resolver *resolver, int chip_num,
				unsigned int *offsets,
				enum gpiod_line_value *values)
{
	struct resolved_line *line;
	int i, num_lines = 0;

	for (i = 0; i < resolver->num_lines; i++) {
		line = &resolver->lines[i];
		if (line->chip_num == chip_num) {
			offsets[num_lines] = line->offset;
			if (values)
				values[num_lines] = line->value;

			num_lines++;
		}
	}

	return num_lines;
}

const char *get_chip_name(struct line_resolver *resolver, int chip_num)
{
	return gpiod_chip_info_get_name(resolver->chips[chip_num].info);
}

const char *get_line_name(struct line_resolver *resolver, int chip_num,
			  unsigned int offset)
{
	struct resolved_line *line;
	int i;

	for (i = 0; i < resolver->num_lines; i++) {
		line = &resolver->lines[i];
		if (line->info && (line->offset == offset) &&
		    (line->chip_num == chip_num))
			return gpiod_line_info_get_name(
				resolver->lines[i].info);
	}

	return 0;
}

void set_line_values(struct line_resolver *resolver, int chip_num,
		     enum gpiod_line_value *values)
{
	int i, j;

	for (i = 0, j = 0; i < resolver->num_lines; i++) {
		if (resolver->lines[i].chip_num == chip_num) {
			resolver->lines[i].value = values[j];
			j++;
		}
	}
}
