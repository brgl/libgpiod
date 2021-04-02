// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017-2021 Bartosz Golaszewski <bartekgola@gmail.com>

/*
 * More specific variants of the core API and misc functions that don't need
 * access to neither the internal library data structures nor the kernel UAPI.
 */

#include <errno.h>
#include <gpiod.h>
#include <stdio.h>
#include <string.h>

#include "internal.h"

GPIOD_API struct gpiod_line_bulk *
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

GPIOD_API struct gpiod_line_bulk *
gpiod_chip_get_all_lines(struct gpiod_chip *chip)
{
	struct gpiod_line_bulk *bulk;
	struct gpiod_line *line;
	unsigned int offset;

	bulk = gpiod_line_bulk_new(gpiod_chip_get_num_lines(chip));
	if (!bulk)
		return NULL;

	for (offset = 0; offset < gpiod_chip_get_num_lines(chip); offset++) {
		line = gpiod_chip_get_line(chip, offset);
		if (!line) {
			gpiod_line_bulk_free(bulk);
			return NULL;
		}

		gpiod_line_bulk_add_line(bulk, line);
	}

	return bulk;
}

GPIOD_API int gpiod_chip_find_line(struct gpiod_chip *chip, const char *name)
{
	unsigned int offset, num_lines;
	struct gpiod_line *line;
	const char *tmp;

	num_lines = gpiod_chip_get_num_lines(chip);

	for (offset = 0; offset < num_lines; offset++) {
		line = gpiod_chip_get_line(chip, offset);
		if (!line)
			return -1;

		tmp = gpiod_line_name(line);
		if (tmp && strcmp(tmp, name) == 0)
			return gpiod_line_offset(line);
	}

	errno = ENOENT;
	return -1;
}

GPIOD_API int gpiod_line_request_input(struct gpiod_line *line,
				       const char *consumer)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT,
	};

	return gpiod_line_request(line, &config, 0);
}

GPIOD_API int gpiod_line_request_output(struct gpiod_line *line,
					const char *consumer, int default_val)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
	};

	return gpiod_line_request(line, &config, default_val);
}

GPIOD_API int gpiod_line_request_input_flags(struct gpiod_line *line,
					     const char *consumer, int flags)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT,
		.flags = flags,
	};

	return gpiod_line_request(line, &config, 0);
}

GPIOD_API int gpiod_line_request_output_flags(struct gpiod_line *line,
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

GPIOD_API int gpiod_line_request_rising_edge_events(struct gpiod_line *line,
						    const char *consumer)
{
	return line_event_request_type(line, consumer, 0,
				       GPIOD_LINE_REQUEST_EVENT_RISING_EDGE);
}

GPIOD_API int gpiod_line_request_falling_edge_events(struct gpiod_line *line,
						     const char *consumer)
{
	return line_event_request_type(line, consumer, 0,
				       GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE);
}

GPIOD_API int gpiod_line_request_both_edges_events(struct gpiod_line *line,
						   const char *consumer)
{
	return line_event_request_type(line, consumer, 0,
				       GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES);
}

GPIOD_API int
gpiod_line_request_rising_edge_events_flags(struct gpiod_line *line,
					    const char *consumer,
					    int flags)
{
	return line_event_request_type(line, consumer, flags,
				       GPIOD_LINE_REQUEST_EVENT_RISING_EDGE);
}

GPIOD_API int
gpiod_line_request_falling_edge_events_flags(struct gpiod_line *line,
					     const char *consumer,
					     int flags)
{
	return line_event_request_type(line, consumer, flags,
				       GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE);
}

GPIOD_API int
gpiod_line_request_both_edges_events_flags(struct gpiod_line *line,
					   const char *consumer, int flags)
{
	return line_event_request_type(line, consumer, flags,
				       GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES);
}

GPIOD_API int gpiod_line_request_bulk_input(struct gpiod_line_bulk *bulk,
					    const char *consumer)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT,
	};

	return gpiod_line_request_bulk(bulk, &config, 0);
}

GPIOD_API int gpiod_line_request_bulk_output(struct gpiod_line_bulk *bulk,
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

GPIOD_API int
gpiod_line_request_bulk_rising_edge_events(struct gpiod_line_bulk *bulk,
					   const char *consumer)
{
	return line_event_request_type_bulk(bulk, consumer, 0,
					GPIOD_LINE_REQUEST_EVENT_RISING_EDGE);
}

GPIOD_API int
gpiod_line_request_bulk_falling_edge_events(struct gpiod_line_bulk *bulk,
					    const char *consumer)
{
	return line_event_request_type_bulk(bulk, consumer, 0,
					GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE);
}

GPIOD_API int
gpiod_line_request_bulk_both_edges_events(struct gpiod_line_bulk *bulk,
					  const char *consumer)
{
	return line_event_request_type_bulk(bulk, consumer, 0,
					GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES);
}

GPIOD_API int gpiod_line_request_bulk_input_flags(struct gpiod_line_bulk *bulk,
						  const char *consumer,
						  int flags)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT,
		.flags = flags,
	};

	return gpiod_line_request_bulk(bulk, &config, 0);
}

GPIOD_API int gpiod_line_request_bulk_output_flags(struct gpiod_line_bulk *bulk,
						   const char *consumer,
						   int flags,
						   const int *default_vals)
{
	struct gpiod_line_request_config config = {
		.consumer = consumer,
		.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
		.flags = flags,
	};

	return gpiod_line_request_bulk(bulk, &config, default_vals);
}

GPIOD_API int gpiod_line_request_bulk_rising_edge_events_flags(
					struct gpiod_line_bulk *bulk,
					const char *consumer, int flags)
{
	return line_event_request_type_bulk(bulk, consumer, flags,
					GPIOD_LINE_REQUEST_EVENT_RISING_EDGE);
}

GPIOD_API int gpiod_line_request_bulk_falling_edge_events_flags(
					struct gpiod_line_bulk *bulk,
					const char *consumer, int flags)
{
	return line_event_request_type_bulk(bulk, consumer, flags,
					GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE);
}

GPIOD_API int gpiod_line_request_bulk_both_edges_events_flags(
					struct gpiod_line_bulk *bulk,
					const char *consumer, int flags)
{
	return line_event_request_type_bulk(bulk, consumer, flags,
					GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES);
}
