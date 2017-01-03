/*
 * GPIO chardev utils for linux.
 *
 * Copyright (C) 2017 Bartosz Golaszewski <bartekgola@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */

/**
 * @mainpage libgpiod public API
 *
 * This is the documentation of the public API exported by libgpiod.
 *
 * <p>These functions and data structures allow to use the complete
 * functionality of the linux GPIO character device interface.
 */

#ifndef __GPIOD__
#define __GPIOD__

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GPIOD_API		__attribute__((visibility("default")))

#define GPIOD_BIT(nr)		(1UL << (nr))

#define __GPIOD_ERRNO_OFFSET	10000

enum {
	GPIOD_ESUCCESS = __GPIOD_ERRNO_OFFSET,
	GPIOD_ENOTREQUESTED,
	__GPIOD_MAX_ERR,
};

int gpiod_errno(void) GPIOD_API;

const char * gpiod_strerror(int errnum) GPIOD_API;

int gpiod_simple_get_value(const char *device, unsigned int offset) GPIOD_API;

enum {
	GPIOD_DIRECTION_IN,
	GPIOD_DIRECTION_OUT,
};

enum {
	GPIOD_POLARITY_ACTIVE_HIGH,
	GPIOD_POLARITY_ACTIVE_LOW,
};

enum {
	GPIOD_REQUEST_DIRECTION_AS_IS		= GPIOD_BIT(0),
	GPIOD_REQUEST_DIRECTION_INPUT		= GPIOD_BIT(1),
	GPIOD_REQUEST_DIRECTION_OUTPUT		= GPIOD_BIT(2),
	GPIOD_REQUEST_POLARITY_ACTIVE_HIGH	= GPIOD_BIT(3),
	GPIOD_REQUEST_POLARITY_ACTIVE_LOW	= GPIOD_BIT(4),
	GPIOD_REQUEST_OPEN_DRAIN		= GPIOD_BIT(5),
	GPIOD_REQUEST_OPEN_SOURCE		= GPIOD_BIT(6),
};

#define GPIOD_MAX_LINES		64

struct gpiod_line;

unsigned int gpiod_line_offset(struct gpiod_line *line) GPIOD_API;

const char * gpiod_line_name(struct gpiod_line *line) GPIOD_API;

const char * gpiod_line_consumer(struct gpiod_line *line) GPIOD_API;

int gpiod_line_direction(struct gpiod_line *line) GPIOD_API;

int gpiod_line_polarity(struct gpiod_line *line) GPIOD_API;

bool gpiod_line_is_used_by_kernel(struct gpiod_line *line) GPIOD_API;

bool gpiod_line_is_open_drain(struct gpiod_line *line) GPIOD_API;

bool gpiod_line_is_open_source(struct gpiod_line *line) GPIOD_API;

int gpiod_line_request(struct gpiod_line *line, const char *consumer,
		       int default_val, int flags) GPIOD_API;

static inline int gpiod_line_request_din(struct gpiod_line *line,
					 const char *consumer, int flags)
{
	return gpiod_line_request(line, consumer, 0,
				  flags | GPIOD_REQUEST_DIRECTION_INPUT);
}

static inline int gpiod_line_request_dout(struct gpiod_line *line,
					  const char *consumer,
					  int default_val, int flags)
{
	return gpiod_line_request(line, consumer, default_val,
				  flags | GPIOD_REQUEST_DIRECTION_OUTPUT);
}

struct gpiod_line_bulk {
	struct gpiod_line *lines[GPIOD_MAX_LINES];
	unsigned int num_lines;
};

#define GPIOD_LINE_BULK_INITIALIZER	{ .num_lines = 0, }

static inline void gpiod_line_bulk_init(struct gpiod_line_bulk *line_bulk)
{
	line_bulk->num_lines = 0;
}

static inline void gpiod_line_bulk_add(struct gpiod_line_bulk *line_bulk,
				       struct gpiod_line *line)
{
	line_bulk->lines[line_bulk->num_lines++] = line;
}

int gpiod_line_request_bulk(struct gpiod_line_bulk *line_bulk,
			    const char *consumer, int *default_vals,
			    int flags) GPIOD_API;

void gpiod_line_release(struct gpiod_line *line) GPIOD_API;

void gpiod_line_release_bulk(struct gpiod_line_bulk *line_bulk) GPIOD_API;

bool gpiod_line_is_requested(struct gpiod_line *line) GPIOD_API;

int gpiod_line_get_value(struct gpiod_line *line) GPIOD_API;

int gpiod_line_get_value_bulk(struct gpiod_line_bulk *line_bulk,
			      int *values) GPIOD_API;

int gpiod_line_set_value(struct gpiod_line *line, int value) GPIOD_API;

int gpiod_line_set_value_bulk(struct gpiod_line_bulk *line_bulk,
			      int *values) GPIOD_API;

struct gpiod_chip;

struct gpiod_chip * gpiod_chip_open(const char *path) GPIOD_API;

struct gpiod_chip * gpiod_chip_open_by_name(const char *name) GPIOD_API;

struct gpiod_chip * gpiod_chip_open_by_number(unsigned int num) GPIOD_API;

struct gpiod_chip * gpiod_chip_open_lookup(const char *descr) GPIOD_API;

void gpiod_chip_close(struct gpiod_chip *chip) GPIOD_API;

const char * gpiod_chip_name(struct gpiod_chip *chip) GPIOD_API;

const char * gpiod_chip_label(struct gpiod_chip *chip) GPIOD_API;

unsigned int gpiod_chip_num_lines(struct gpiod_chip *chip) GPIOD_API;

struct gpiod_line *
gpiod_chip_get_line(struct gpiod_chip *chip, unsigned int offset) GPIOD_API;

int gpiod_chip_get_fd(struct gpiod_chip *chip) GPIOD_API;

struct gpiod_chip * gpiod_line_get_chip(struct gpiod_line *line) GPIOD_API;

struct gpiod_chip_iter;

struct gpiod_chip_iter * gpiod_chip_iter_new(void) GPIOD_API;

void gpiod_chip_iter_free(struct gpiod_chip_iter *iter) GPIOD_API;

struct gpiod_chip *
gpiod_chip_iter_next(struct gpiod_chip_iter *iter) GPIOD_API;

#define gpiod_foreach_chip(iter, chip)					\
	for ((chip) = gpiod_chip_iter_next(iter);			\
	     (chip);							\
	     (chip) = gpiod_chip_iter_next(iter))

struct gpiod_line_iter {
	unsigned int offset;
};

#define GPIOD_LINE_ITER_INITIALIZER	{ 0 }

static inline void gpiod_line_iter_init(struct gpiod_line_iter *iter)
{
	iter->offset = 0;
}

static inline struct gpiod_line *
gpiod_chip_line_next(struct gpiod_chip *chip, struct gpiod_line_iter *iter)
{
	if (iter->offset >= gpiod_chip_num_lines(chip))
		return NULL;

	return gpiod_chip_get_line(chip, iter->offset++);
}

#define gpiod_chip_foreach_line(iter, chip, line)			\
	for ((line) = gpiod_chip_line_next(chip, iter);			\
	     (line);							\
	     (line) = gpiod_chip_line_next(chip, iter))

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __GPIOD__ */
