// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2017-2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

/*
 * More specific variants of the core API and misc functions that don't need
 * access to neither the internal library data structures nor the kernel UAPI.
 */

#include <ctype.h>
#include <errno.h>
#include <gpiod.h>
#include <stdio.h>
#include <string.h>

static bool isuint(const char *str)
{
	for (; *str && isdigit(*str); str++)
		;

	return *str == '\0';
}

struct gpiod_chip *gpiod_chip_open_by_name(const char *name)
{
	struct gpiod_chip *chip;
	char *path;
	int rv;

	rv = asprintf(&path, "/dev/%s", name);
	if (rv < 0)
		return NULL;

	chip = gpiod_chip_open(path);
	free(path);

	return chip;
}

struct gpiod_chip *gpiod_chip_open_by_number(unsigned int num)
{
	struct gpiod_chip *chip;
	char *path;
	int rv;

	rv = asprintf(&path, "/dev/gpiochip%u", num);
	if (!rv)
		return NULL;

	chip = gpiod_chip_open(path);
	free(path);

	return chip;
}

struct gpiod_chip *gpiod_chip_open_by_label(const char *label)
{
	struct gpiod_chip_iter *iter;
	struct gpiod_chip *chip;

	iter = gpiod_chip_iter_new();
	if (!iter)
		return NULL;

	gpiod_foreach_chip(iter, chip) {
		if (strcmp(label, gpiod_chip_label(chip)) == 0) {
			gpiod_chip_iter_free_noclose(iter);
			return chip;
		}
	}

	errno = ENOENT;
	gpiod_chip_iter_free(iter);

	return NULL;
}

struct gpiod_chip *gpiod_chip_open_lookup(const char *descr)
{
	struct gpiod_chip *chip;

	if (isuint(descr)) {
		chip = gpiod_chip_open_by_number(strtoul(descr, NULL, 10));
	} else {
		chip = gpiod_chip_open_by_label(descr);
		if (!chip) {
			if (strncmp(descr, "/dev/", 5))
				chip = gpiod_chip_open_by_name(descr);
			else
				chip = gpiod_chip_open(descr);
		}
	}

	return chip;
}

struct gpiod_line_bulk *
gpiod_chip_get_lines(struct gpiod_chip *chip,
		     unsigned int *offsets, unsigned int num_offsets)
{
	struct gpiod_line_bulk *bulk;
	struct gpiod_line *line;
	unsigned int i;

	bulk = gpiod_line_bulk_new(num_offsets);
	if (!bulk)
		return NULL;

	for (i = 0; i < num_offsets; i++) {
		line = gpiod_chip_get_line(chip, offsets[i]);
		if (!line) {
			gpiod_line_bulk_free(bulk);
			return NULL;
		}

		gpiod_line_bulk_add_line(bulk, line);
	}

	return bulk;
}

struct gpiod_line_bulk *gpiod_chip_get_all_lines(struct gpiod_chip *chip)
{
	struct gpiod_line_bulk *bulk;
	struct gpiod_line *line;
	unsigned int offset;

	bulk = gpiod_line_bulk_new(gpiod_chip_num_lines(chip));
	if (!bulk)
		return NULL;

	for (offset = 0; offset < gpiod_chip_num_lines(chip); offset++) {
		line = gpiod_chip_get_line(chip, offset);
		if (!line) {
			gpiod_line_bulk_free(bulk);
			return NULL;
		}

		gpiod_line_bulk_add_line(bulk, line);
	}

	return bulk;
}

struct gpiod_line *
gpiod_chip_find_line(struct gpiod_chip *chip, const char *name)
{
	struct gpiod_line *line;
	unsigned int offset;
	const char *tmp;

	for (offset = 0; offset < gpiod_chip_num_lines(chip); offset++) {
		line = gpiod_chip_get_line(chip, offset);
		if (!line)
			return NULL;

		tmp = gpiod_line_name(line);
		if (tmp && strcmp(tmp, name) == 0)
			return line;
	}

	errno = ENOENT;

	return NULL;
}

struct gpiod_line_bulk *
gpiod_chip_find_lines(struct gpiod_chip *chip, const char **names)
{
	struct gpiod_line_bulk *bulk;
	struct gpiod_line *line;
	unsigned int num_names;
	int i;

	for (i = 0; names[i]; i++);
	num_names = i;

	bulk = gpiod_line_bulk_new(num_names);
	if (!bulk)
		return NULL;

	for (i = 0; names[i]; i++) {
		line = gpiod_chip_find_line(chip, names[i]);
		if (!line) {
			gpiod_line_bulk_free(bulk);
			return NULL;
		}

		gpiod_line_bulk_add_line(bulk, line);
	}

	return bulk;
}

int gpiod_line_request_input(struct gpiod_line *line, const char *consumer)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT,
	};

	return gpiod_line_request(line, &config, 0);
}

int gpiod_line_request_output(struct gpiod_line *line,
			      const char *consumer, int default_val)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
	};

	return gpiod_line_request(line, &config, default_val);
}

int gpiod_line_request_input_flags(struct gpiod_line *line,
				   const char *consumer, int flags)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT,
		.flags = flags,
	};

	return gpiod_line_request(line, &config, 0);
}

int gpiod_line_request_output_flags(struct gpiod_line *line,
				    const char *consumer, int flags,
				    int default_val)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
		.flags = flags,
	};

	return gpiod_line_request(line, &config, default_val);
}

static int line_event_request_type(struct gpiod_line *line,
				   const char *consumer, int flags, int type)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = type,
		.flags = flags,
	};

	return gpiod_line_request(line, &config, 0);
}

int gpiod_line_request_rising_edge_events(struct gpiod_line *line,
					  const char *consumer)
{
	return line_event_request_type(line, consumer, 0,
				       GPIOD_LINE_REQUEST_EVENT_RISING_EDGE);
}

int gpiod_line_request_falling_edge_events(struct gpiod_line *line,
					   const char *consumer)
{
	return line_event_request_type(line, consumer, 0,
				       GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE);
}

int gpiod_line_request_both_edges_events(struct gpiod_line *line,
					 const char *consumer)
{
	return line_event_request_type(line, consumer, 0,
				       GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES);
}

int gpiod_line_request_rising_edge_events_flags(struct gpiod_line *line,
						const char *consumer,
						int flags)
{
	return line_event_request_type(line, consumer, flags,
				       GPIOD_LINE_REQUEST_EVENT_RISING_EDGE);
}

int gpiod_line_request_falling_edge_events_flags(struct gpiod_line *line,
						 const char *consumer,
						 int flags)
{
	return line_event_request_type(line, consumer, flags,
				       GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE);
}

int gpiod_line_request_both_edges_events_flags(struct gpiod_line *line,
					       const char *consumer, int flags)
{
	return line_event_request_type(line, consumer, flags,
				       GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES);
}

int gpiod_line_request_bulk_input(struct gpiod_line_bulk *bulk,
				  const char *consumer)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT,
	};

	return gpiod_line_request_bulk(bulk, &config, 0);
}

int gpiod_line_request_bulk_output(struct gpiod_line_bulk *bulk,
				   const char *consumer,
				   const int *default_vals)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
	};

	return gpiod_line_request_bulk(bulk, &config, default_vals);
}

static int line_event_request_type_bulk(struct gpiod_line_bulk *bulk,
					const char *consumer,
					int flags, int type)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = type,
		.flags = flags,
	};

	return gpiod_line_request_bulk(bulk, &config, 0);
}

int gpiod_line_request_bulk_rising_edge_events(struct gpiod_line_bulk *bulk,
					       const char *consumer)
{
	return line_event_request_type_bulk(bulk, consumer, 0,
					GPIOD_LINE_REQUEST_EVENT_RISING_EDGE);
}

int gpiod_line_request_bulk_falling_edge_events(struct gpiod_line_bulk *bulk,
						const char *consumer)
{
	return line_event_request_type_bulk(bulk, consumer, 0,
					GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE);
}

int gpiod_line_request_bulk_both_edges_events(struct gpiod_line_bulk *bulk,
					      const char *consumer)
{
	return line_event_request_type_bulk(bulk, consumer, 0,
					GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES);
}

int gpiod_line_request_bulk_input_flags(struct gpiod_line_bulk *bulk,
					const char *consumer, int flags)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT,
		.flags = flags,
	};

	return gpiod_line_request_bulk(bulk, &config, 0);
}

int gpiod_line_request_bulk_output_flags(struct gpiod_line_bulk *bulk,
					 const char *consumer, int flags,
					 const int *default_vals)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
		.flags = flags,
	};

	return gpiod_line_request_bulk(bulk, &config, default_vals);
}

int gpiod_line_request_bulk_rising_edge_events_flags(
					struct gpiod_line_bulk *bulk,
					const char *consumer, int flags)
{
	return line_event_request_type_bulk(bulk, consumer, flags,
					GPIOD_LINE_REQUEST_EVENT_RISING_EDGE);
}

int gpiod_line_request_bulk_falling_edge_events_flags(
					struct gpiod_line_bulk *bulk,
					const char *consumer, int flags)
{
	return line_event_request_type_bulk(bulk, consumer, flags,
					GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE);
}

int gpiod_line_request_bulk_both_edges_events_flags(
					struct gpiod_line_bulk *bulk,
					const char *consumer, int flags)
{
	return line_event_request_type_bulk(bulk, consumer, flags,
					GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES);
}

struct gpiod_line *gpiod_line_get(const char *device, unsigned int offset)
{
	struct gpiod_chip *chip;
	struct gpiod_line *line;

	chip = gpiod_chip_open_lookup(device);
	if (!chip)
		return NULL;

	line = gpiod_chip_get_line(chip, offset);
	if (!line) {
		gpiod_chip_close(chip);
		return NULL;
	}

	return line;
}

struct gpiod_line *gpiod_line_find(const char *name)
{
	struct gpiod_chip_iter *iter;
	struct gpiod_chip *chip;
	struct gpiod_line *line;

	iter = gpiod_chip_iter_new();
	if (!iter)
		return NULL;

	gpiod_foreach_chip(iter, chip) {
		line = gpiod_chip_find_line(chip, name);
		if (line) {
			gpiod_chip_iter_free_noclose(iter);
			return line;
		}

		if (errno != ENOENT)
			goto out;
	}

	errno = ENOENT;

out:
	gpiod_chip_iter_free(iter);

	return NULL;
}

void gpiod_line_close_chip(struct gpiod_line *line)
{
	struct gpiod_chip *chip = gpiod_line_get_chip(line);

	gpiod_chip_close(chip);
}
